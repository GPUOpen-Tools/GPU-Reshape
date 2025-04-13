// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

// Feature
#include <Features/Debug/Feature.h>
#include <Features/Debug/BreakpointType.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/Emitters/ResourceTokenEmitter.h>
#include <Backend/IL/InstructionValueCommon.h>
#include <Backend/CommandContext.h>
#include <Backend/SubmissionContext.h>
#include <Backend/Command/CommandBuilder.h>
#include <Backend/IL/Emitters/ExtendedEmitter.h>
#include <Backend/IL/Metadata/KernelMetadata.h>
#include <Backend/Scheduler/IScheduler.h>
#include <Backend/Scheduler/SchedulerTileMapping.h>

// Generated schema
#include <Schemas/Features/Debug.h>
#include <Schemas/Features/DebugConfig.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Common
#include <Common/FileSystem.h>
#include <Common/Registry.h>

/// TODO[dbg]: Temporary work
static constexpr uint32_t kWidth = 1920;
static constexpr uint32_t kHeight = 1080;

/// Maximum streaming size
static constexpr uint64_t kDebugStreamBufferSize = UINT32_MAX; // ~4gb

bool DebugFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Allocate the shared export
    exportID = exportHost->Allocate<DebugAssertMessage>();

    // Optional sguid host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // Get scheduler
    scheduler = registry->Get<IScheduler>();

    // Create monotonic primitive
    exclusiveTransferPrimitiveID = scheduler->CreatePrimitive();

    // Create tiled streaming buffer
    streamBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = kDebugStreamBufferSize / sizeof(uint32_t),
        .format = Backend::IL::Format::R32UInt,
        .flagSet = ShaderDataBufferFlag::Tiled
    }, "DebugStreamBuffer");

    // Create allocator for the streaming buffer
    buddyAllocator.Install(kDebugStreamBufferSize + 1);

    // Create residency handler for the streaming buffer
    tileResidencyAllocator.Install(kDebugStreamBufferSize);

    // OK
    return true;
}

FeatureHookTable DebugFeature::GetHookTable() {
    FeatureHookTable table{};
    table.preSubmit = BindDelegate(this, DebugFeature::OnSubmitBatchBegin);
    table.syncPoint = BindDelegate(this, DebugFeature::OnSyncPoint);
    return table;
}

void DebugFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void DebugFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void DebugFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    std::unordered_map<uint32_t, DebugBreakpointMessage> breakpointStreams;

    // Options
    if (const DebugConfigMessage* debugConfig = Find<DebugConfigMessage>(specialization)) {
        // Parse all breakpoints
        for (auto it = ConstMessageStreamView<DebugBreakpointMessage, MessageSubStream>(debugConfig->breakpoints).GetIterator(); it; ++it) {
            breakpointStreams[it->codeOffset] = *it.Get();
        }
    }

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // No reason to instrument terminators
        // Also conveniently hide issue with pointing past terminators
        if (it == context.basicBlock.GetTerminator()) {
            return it;
        }
        
        if (auto breakpointIt = breakpointStreams.find(it->source.codeOffset); breakpointIt != breakpointStreams.end()) {
            return InjectBreakpoint(context, it, breakpointIt->second);
        }

        // TODO[dbg]: Let's not reiterate the everything 
        return it;
    });
}

void DebugFeature::Handle(const MessageStream *streams, uint32_t count) {
    std::lock_guard guard(mutex);

    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView view(streams[i]);

        // Visit all ordered messages
        for (ConstMessageStreamView<>::ConstIterator it = view.GetIterator(); it; ++it) {
            switch (it.GetID()) {
                case DebugBreakpointStreamHandledMessage::kID: {
                    const DebugBreakpointStreamHandledMessage *msg = it.Get<DebugBreakpointStreamHandledMessage>();
                    defaultController.remoteRequestIndex = msg->request;
                    break;
                }
                case RegisterDebugBreakpointMessage::kID: {
                    const RegisterDebugBreakpointMessage *msg = it.Get<RegisterDebugBreakpointMessage>();

                    // Add breakpoint
                    Breakpoint& breakpoint = breakpoints.emplace_back();
                    breakpoint.uid = msg->uid;
                    breakpoint.type = static_cast<BreakpointType>(msg->type);

                    // Create payload
                    switch (breakpoint.type) {
                        case BreakpointType::Image: {
                            breakpoint.payload.image.width = kWidth;
                            breakpoint.payload.image.height = kHeight;
                            breakpoint.streamSize = breakpoint.payload.image.width * breakpoint.payload.image.height * sizeof(uint32_t);
                            break;
                        }
                        case BreakpointType::Data: {
                            break;
                        }
                    }

                    // Allocate the underlying memory
                    breakpoint.allocation = buddyAllocator.Allocate(breakpoint.streamSize);

                    // Map the relevant times for the range
                    tileResidencyAllocator.Allocate(
                        breakpoint.allocation.offset,
                        breakpoint.allocation.length
                    );

                    // Create streaming counter-part
                    breakpoint.hostStreamingBuffer = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
                        .elementCount = breakpoint.streamSize,
                        .format = Backend::IL::Format::R8UInt,
                        .flagSet = ShaderDataBufferFlag::Host
                    }, "DebugStreamHost");
                    
                    break;
                }
                case DeregisterDebugBreakpointMessage::kID: {
                    const DeregisterDebugBreakpointMessage *msg = it.Get<DeregisterDebugBreakpointMessage>();

                    // Find the matching breakpoint
                    for (auto breakpointIt = breakpoints.begin(); breakpointIt != breakpoints.end(); ++breakpointIt) {
                        if (breakpointIt->uid == msg->uid) {
                            breakpoints.erase(breakpointIt);
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
}

void DebugFeature::OnSubmitBatchBegin(SubmissionContext &submitContext, const CommandContextHandle *contexts, uint32_t contextCount) {
    std::lock_guard guard(mutex);

    // Any tiles pending mapping?
    if (tileResidencyAllocator.GetRequestCount()) {
        // Allocate the next sync value
        ++exclusiveTransferPrimitiveMonotonicCounter;
        
        // All mappings
        std::vector<SchedulerTileMapping> tileMappings;
        tileMappings.reserve(tileResidencyAllocator.GetRequestCount());

        // Map all new requests
        for (uint32_t i = 0; i < tileResidencyAllocator.GetRequestCount(); i++) {
            const TileMappingRequest& request = tileResidencyAllocator.GetRequest(i);

            // Create mapping and push for mapping
            tileMappings.push_back(SchedulerTileMapping {
                .mapping = shaderDataHost->CreateMapping(streamBufferID, request.tileCount),
                .tileOffset = request.tileOffset,
                .tileCount = request.tileCount
            });
        }

        // Cleanup
        tileResidencyAllocator.ClearRequests();

        // Create the tile mappings for the new resource
        scheduler->MapTiles(Queue::ExclusiveTransfer, streamBufferID, static_cast<uint32_t>(tileMappings.size()), tileMappings.data());

        // Submit to the transfer queue
        SchedulerPrimitiveEvent event;
        event.id = exclusiveTransferPrimitiveID;
        event.value = exclusiveTransferPrimitiveMonotonicCounter;
        scheduler->Schedule(Queue::ExclusiveTransfer, CommandBuffer {}, &event);
    }

    // Submissions always wait for the last mappings
    submitContext.waitPrimitives.Add(SchedulerPrimitiveEvent {
        .id = exclusiveTransferPrimitiveID,
        .value = exclusiveTransferPrimitiveMonotonicCounter
    });

    CommandBuilder builder(submitContext.postContext->buffer);
    
    // Copy the debug streaming buffer to host
    for (const Breakpoint& breakpoint : breakpoints) {
        // TODO[dbg]: This is incorrect, of course
        builder.CopyBuffer(
            streamBufferID, breakpoint.allocation.offset,
            breakpoint.hostStreamingBuffer, 0,
            breakpoint.streamSize
        );
    }
}

bool DebugFeature::ThrottleController(RequestController &controller) {
    TimePoint now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now());

    // Passed scheduled interval?
    if (now - controller.lastStreamBufferTime < Duration(static_cast<uint32_t>(controller.streamBufferInterval))) {
        return false;
    }

    // Determine the number of in-flight requests
    uint32_t window = controller.requestIndex - controller.remoteRequestIndex;

    // TODO[dbg]: This "works", but the magic constants aren't that good
    if (window > kRequestThrottleWindow) {
        controller.streamBufferInterval *= 1.2;
    } else {
        controller.streamBufferInterval *= 0.8;
    }

    // Set new streaming interval, clamped to bounds
    controller.lastStreamBufferTime = now;
    controller.streamBufferInterval = std::clamp(controller.streamBufferInterval, kMinDurationBeforeThrottle, kMaxDurationBeforeDrop);

    // OK
    return true;
}

void DebugFeature::OnSyncPoint() {
    std::lock_guard guard(mutex);

    // No breakpoints? No data
    if (breakpoints.empty()) {
        return;
    }

    // Throttle the data requests
    if (!ThrottleController(defaultController)) {
        return;
    }

    // Stream out the breakpoints separately
    for (const Breakpoint& breakpoint : breakpoints) {
        // Map the streaming buffer
        void* data = shaderDataHost->Map(breakpoint.hostStreamingBuffer);

        // Empty out last stream
        MessageStreamView<DebugBreakpointStreamMessage> view(stream);
        stream.Clear();

        // Allocate breakpoint data
        auto message = view.Add(DebugBreakpointStreamMessage::AllocationInfo {
            .dataCount = breakpoint.streamSize
        });

        // TODO[dbg]: Can we somehow map this in-place? There's a lot of copies going on
        std::memcpy(message->data.Get(), data, breakpoint.streamSize);

        // Write out request data
        message->request = ++defaultController.requestIndex;
        message->type = 0;
        message->dataType = 0;
        message->uid = breakpoint.uid;
        message->width = breakpoint.payload.image.width;
        message->height = breakpoint.payload.image.height;

        // Done!
        shaderDataHost->Unmap(breakpoint.hostStreamingBuffer, data);
    }
}

IL::ID DebugFeature::GetStaticOrderingFor(const IL::VisitContext &context, IL::Emitter<>& emitter, const IL::Instruction* it) {
    auto* md = context.program.GetMetadataMap().GetMetadata<IL::KernelTypeMetadata>(context.function.GetID());

    // Right now only supporting compute
    if (!md || md->type != IL::KernelType::Compute) {
        return IL::InvalidID;
    }

    // Get typed dispatch index
    IL::ID dtid = emitter.KernelValue(Backend::IL::KernelValue::DispatchThreadID);

    // Get dimensions
    IL::ID x = emitter.Extract(dtid, emitter.UInt32(0));
    IL::ID y = emitter.Extract(dtid, emitter.UInt32(1));

    // Get guard mask against the expected bounds
    IL::ID mask = emitter.LessThan(x, emitter.UInt32(kWidth));
    mask = emitter.And(mask, emitter.LessThan(y, emitter.UInt32(kHeight)));

    // Dumb selection
    return emitter.Select(
        mask,
        emitter.Add(emitter.Mul(y, emitter.UInt32(kWidth)), x),
        emitter.UInt32(64000)
    );
}

IL::BasicBlock::Iterator DebugFeature::SplitInterruptBlock(const IL::VisitContext& context, IL::BasicBlock::Iterator it, IL::BasicBlock *interruptBlock) {
    // Split the iterator to resume
    // Excluding the iterator itself, since we may want to reference the instruction results
    IL::BasicBlock* resumeBlock = context.function.GetBasicBlocks().AllocBlock();
    it.block->Split(resumeBlock, std::next(it));

    // Branch the split-end to interrupt
    IL::Emitter(context.program, *it.block).Branch(interruptBlock);
    
    // Branch the resume to interrupt
    IL::Emitter(context.program, *interruptBlock).Branch(resumeBlock);
    return resumeBlock->begin();
}

static bool IsTypeSupported(const Backend::IL::Type* type) {
    switch (type->kind) {
        default: {
            return false;
        }
        case Backend::IL::TypeKind::Bool:
        case Backend::IL::TypeKind::Int:
        case Backend::IL::TypeKind::FP: {
            return true;
        }
        case Backend::IL::TypeKind::Vector: {
            auto typed = type->As<Backend::IL::VectorType>();
            return IsTypeSupported(typed->containedType) && typed->dimension <= 4;
        }
        case Backend::IL::TypeKind::Struct: {
            auto typed = type->As<Backend::IL::StructType>();

            for (const Backend::IL::Type *member: typed->memberTypes) {
                if (!IsTypeSupported(member)) {
                    return false;
                }
            }

            return true;
        }
    }
}

static IL::ID GetDataPixel(const IL::VisitContext &context, IL::Emitter<> emitter, IL::ID value) {
    const Backend::IL::Type *type = context.program.GetTypeMap().GetType(value);

    // Extended emitter for common operations
    IL::ExtendedEmitter extended(emitter);

    // Handle type
    switch (type->kind) {
        default: {
            ASSERT(false, "Invalid");
            return IL::InvalidID;
        }
        case Backend::IL::TypeKind::Vector: {
            auto* typed = type->As<Backend::IL::VectorType>();

            // Compose color byte by byte
            IL::ID pixel = emitter.UInt32(0);
            for (uint32_t i = 0; i < typed->dimension; i++) {
                IL::ID component = GetDataPixel(context, emitter, emitter.Extract(value, emitter.UInt32(i)));
                pixel = emitter.BitOr(pixel, emitter.BitShiftLeft(component, emitter.UInt32(i * 8)));
            }
            return pixel;
        }
        case Backend::IL::TypeKind::Struct: {
            auto* typed = type->As<Backend::IL::StructType>();

            // Compose color byte by byte
            IL::ID pixel = emitter.UInt32(0);
            for (uint32_t i = 0; i < typed->memberTypes.size(); i++) {
                IL::ID component = GetDataPixel(context, emitter, emitter.Extract(value, emitter.UInt32(i)));
                pixel = emitter.BitOr(pixel, emitter.BitShiftLeft(component, emitter.UInt32(i * 8)));
            }
            return pixel;
        }
        case Backend::IL::TypeKind::FP: {
            // TODO[dbg]: Color space handling
            IL::ID linear = ((IL::ID)emitter.Mul(extended.Pow(value, context.program.GetConstants().FP(0.4545454545f)->id), context.program.GetConstants().FP(255.0f)->id));
            IL::ID quant = emitter.FloatToUInt32(linear); // This is my quant
            return extended.Min(quant, emitter.UInt32(255));
        }
        case Backend::IL::TypeKind::Int:  {
            value = emitter.BitCast(value, context.program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{ .bitWidth = 32 }));
            return extended.Min(value, emitter.UInt32(255));
        }
        case Backend::IL::TypeKind::Bool: {
            return emitter.Select(value, emitter.UInt32(255), emitter.UInt32(0));
        }
    }
}

static IL::ID GetInstructionDebugValue(const IL::Instruction* instr) {
    // Select the value for debugging
    switch (instr->opCode) {
        default:
            return instr->result;
        case IL::OpCode::Store:
            return instr->As<IL::StoreInstruction>()->value;
        case IL::OpCode::StoreTexture:
            return instr->As<IL::StoreTextureInstruction>()->texel;
        case IL::OpCode::StoreBuffer:
            return instr->As<IL::StoreBufferInstruction>()->value;
        case IL::OpCode::StoreBufferRaw:
            return instr->As<IL::StoreBufferRawInstruction>()->value;
        case IL::OpCode::StorePrimitiveOutput:
            return instr->As<IL::StorePrimitiveOutputInstruction>()->value;
        case IL::OpCode::StoreVertexOutput:
            return instr->As<IL::StoreVertexOutputInstruction>()->value;
        case IL::OpCode::StoreOutput:
            return instr->As<IL::StoreOutputInstruction>()->value;
    }
}

IL::BasicBlock::Iterator DebugFeature::InjectBreakpoint(const IL::VisitContext &context, const IL::BasicBlock::Iterator &it, DebugBreakpointMessage breakpoint) {
    // 
    // ACQUIRE FIRST INVOCATION
    // bIs = false
    // if (debugMd.uid == invocationUID)
    //   bIs = true
    // else:
    //   bIs = interlockedOr(debugMd.uid, invocationUID)
    //
    // WRITE MD
    // interlockedMax(debugMd.maxDispatch.xyz, DTID.xyz)
    //
    // WRITE DATA
    // debugMd[order] = X
    //

    // Emit in the interrupt block
    IL::BasicBlock* interruptBlock = context.function.GetBasicBlocks().AllocBlock();
    IL::Emitter<> emitter(context.program, *interruptBlock);

    // Get the value to be emitted
    IL::ID value = GetInstructionDebugValue(it);

    // Check if the type is supported
    const Backend::IL::Type *type = context.program.GetTypeMap().GetType(value);
    if (!type || !IsTypeSupported(type)) {
        return it;
    }

    // Get the export order
    IL::ID order = GetStaticOrderingFor(context, emitter, it);
    if (order == IL::InvalidID) {
        return it;
    }

    // Get the tiled allocation offset
    IL::ID allocationOffset;
    {
        std::lock_guard guard(mutex);

        // Find the relevant breakpoint
        const Breakpoint* candidate = nullptr;
        for (const Breakpoint& _candidate : breakpoints) {
            if (_candidate.uid == breakpoint.uid) {
                candidate = &_candidate;
                break;
            }
        }

        // Shouldn't happen
        if (!candidate) {
            ASSERT(false, "Invalid candidate");
            return it;
        }

        // Set the dword offset
        allocationOffset = emitter.UInt32(static_cast<uint32_t>(candidate->allocation.offset / sizeof(uint32_t)));
    }
    
    // Get the data ids
    IL::ID streamDataID = context.program.GetShaderDataMap().Get(streamBufferID)->id;

    // Compose the pixel colors
    IL::ID pixel = GetDataPixel(context, emitter, value);

    // 255 Alpha
    pixel = emitter.BitOr(pixel, emitter.UInt32(0xFF << 24));

    // Finally, store it
    emitter.StoreBuffer(
        emitter.Load(streamDataID),
        emitter.Add(allocationOffset, order),
        pixel
    );

    // Interrupt the block
    return SplitInterruptBlock(context, it, interruptBlock);
}

FeatureInfo DebugFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Debug";
    info.description = "";
    return info;
}
