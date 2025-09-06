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
#include <Features/Debug/BreakpointHeader.h>
#include <Features/Debug/ChecksumShaderProgram.h>

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
#include <Backend/IL/ShaderStruct.h>
#include <Backend/IL/ShaderBufferStruct.h>
#include <Backend/ShaderProgram/IShaderProgramHost.h>
#include <Backend/Device/DeviceState.h>
#include <Backend/Device/DeviceStateRef.h>
#include <Backend/IL/Tiny/TinyType.h>
#include <Backend/IL/Tiny/TinyTypePacking.h>
#include <Backend/IL/TypeSize.h>

// Generated schema
#include <Schemas/Features/Debug.h>
#include <Schemas/Features/DebugConfig.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Bridge
#include <Bridge/IBridge.h>

// Common
#include <Common/FileSystem.h>
#include <Common/Registry.h>

/// Max number of breakpoints, TODO[dbg]: for now?
static constexpr uint32_t kMaxBreakpoints = 1 << 16;

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
    auto messageType = ShaderExportTypeInfo::FromType<BreakpointAcquisitionMessage>();
    messageType.streamType = ShaderExportStreamType::Input;
    exportID = exportHost->Allocate(messageType);

    // Optional sguid host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // Get scheduler
    scheduler = registry->Get<IScheduler>();

    // Create monotonic primitive
    exclusiveTransferPrimitiveID = scheduler->CreatePrimitive();

    // Create lifetime queue
    contextLifetimeQueue.Install(scheduler);

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
    
#if 0 // TODO[dbg]: Use or not?
    // Must have program host
    auto programHost = registry->Get<IShaderProgramHost>();
    if (!programHost) {
        return false;
    }

    // Create the signal program
    patchShaderProgram = registry->New<ChecksumShaderProgram>(streamBufferID);
    if (!patchShaderProgram->Install()) {
        return false;
    }

    // Register signaller
    patchShaderProgramID = programHost->Register(patchShaderProgram);
#endif

    // Register for messages
    bridge = registry->Get<IBridge>().GetUnsafe();
    bridge->Register(BreakpointAcquisitionMessage::kID, this);

    // Get state voter, used primarily for scheduler changes
    stateVote = registry->Get<IDeviceStateVote>();

    // Allocate breakpoint headers
    buddyAllocator.Allocate(kMaxBreakpoints * sizeof(BreakpointHeader));
    tileResidencyAllocator.Allocate(0, kMaxBreakpoints * sizeof(BreakpointHeader));
    
    // OK
    return true;
}

FeatureHookTable DebugFeature::GetHookTable() {
    FeatureHookTable table{};
    table.preSubmit = BindDelegate(this, DebugFeature::OnPreSubmit);
    table.syncPoint = BindDelegate(this, DebugFeature::OnSyncPoint);
    table.join = BindDelegate(this, DebugFeature::OnJoin);
    return table;
}

void DebugFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void DebugFeature::CollectMessages(IMessageStorage *storage) {
    
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
    
    // Command buffer for stages
    CommandBuffer  buffer;
    CommandBuilder builder(buffer);
    
    for (uint32_t i = 0; i < count; i++) {
        // Handle GPU feedback
        // TODO[dbg]: This is ugly
        if (streams[i].GetSchema().type == MessageSchemaType::Chunked) {
            for (auto it = ConstMessageStreamView<BreakpointAcquisitionMessage>(streams[i]).GetIterator(); it; ++it) {
                OnBreakpointAcquired(it.Get(), builder);
            }
            continue;
        }
        
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
                    breakpoint.captureMode = static_cast<BreakpointCaptureMode>(msg->captureMode);
                    breakpoint.streamSize = msg->streamSize;

                    // Setup payload
                    CreateAndUpdatePayload(breakpoint);

                    // Assign breakpoint device states
                    if (!poolingState.IsSet()) {
                        // Greatly increase pooling rate, speeds up breakpoint streaming
                        poolingState = DeviceStateRef(stateVote.GetUnsafe(), DeviceStatePooling {
                            .intervalMS = 1
                        });
                    }
                    
                    break;
                }
                case ReallocateDebugBreakpointMessage::kID: {
                    const ReallocateDebugBreakpointMessage *msg = it.Get<ReallocateDebugBreakpointMessage>();

                    // Try to find it
                    Breakpoint* breakpoint = FindBreakpointNoLock(msg->uid);
                    if (!breakpoint) {
                        break;
                    }

                    // Push the old allocation to the queue
                    allocationDestructionQueue.push_back(PendingDestruction {
                        .allocation = breakpoint->streamAllocation,
                        .hostStreamingBuffer = breakpoint->hostStreamingBuffer,
                        .lastCommit = contextLifetimeQueue.GetCommitHead()
                    });

                    // Set new streaming size
                    breakpoint->streamSize = msg->streamSize;

                    // Setup payload
                    CreateAndUpdatePayload(*breakpoint);

                    // Assign breakpoint device states
                    if (!poolingState.IsSet()) {
                        // Greatly increase pooling rate, speeds up breakpoint streaming
                        poolingState = DeviceStateRef(stateVote.GetUnsafe(), DeviceStatePooling {
                            .intervalMS = 1
                        });
                    }
                    
                    break;
                }
                case DeregisterDebugBreakpointMessage::kID: {
                    const DeregisterDebugBreakpointMessage *msg = it.Get<DeregisterDebugBreakpointMessage>();

                    // TODO: This isn't really correct, since we're also not waiting for the instrumentation to commit the old stuff, and the pending submissions...
                    // Tricky area to get right. We could have an "invalidated" header region, since the memory is technically still valid for a bit, but not sure.

                    // Find breakpoint
                    auto breakpoint = std::ranges::find_if(breakpoints, [&](const Breakpoint& candidate) {
                        return candidate.uid == msg->uid;
                    });

                    // Shouldn't happen, but just in case
                    if (breakpoint == breakpoints.end()) {
                        break;
                    }

                    // Free its memory
                    allocationDestructionQueue.push_back(PendingDestruction {
                        .allocation = breakpoint->streamAllocation,
                        .hostStreamingBuffer = breakpoint->hostStreamingBuffer,
                        .lastCommit = contextLifetimeQueue.GetCommitHead()
                    });

                    // No longer tracked
                    breakpoints.erase(breakpoint);

                    // Reset breakpoint device states
                    if (breakpoints.empty()) {
                        poolingState = {};
                    }
                    break;
                }
            }
        }
    }
}

void DebugFeature::OnPreSubmit(SubmissionContext &submitContext, const CommandContextHandle *contexts, uint32_t contextCount) {
    std::lock_guard guard(mutex);

    // Anything to sync?
    bool hasSyncRequest = false;

    // Any tiles pending mapping?
    if (tileResidencyAllocator.GetRequestCount()) {
        hasSyncRequest = true;
        
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
    }

    // No breakpoints, let's not do redundant work
    if (breakpoints.empty() && !hasSyncRequest) {
        return;
    }

    // Sync buffer
    CommandBuffer syncBuffer;
    CommandBuilder syncBuilder(syncBuffer);

    // Handle header mappings
    for (Breakpoint& breakpoint : breakpoints) {
        if (!breakpoint.pendingHeader) {
            continue;
        }

        // Reset the header
        syncBuilder.StageBuffer(streamBufferID, breakpoint.uid * sizeof(BreakpointHeader), sizeof(BreakpointHeader), &breakpoint.header);
        breakpoint.pendingHeader = false;
    }

    if (hasSyncRequest) {
        // Allocate the next sync value
        ++exclusiveTransferPrimitiveMonotonicCounter;
        
        // Submit to the transfer queue
        SchedulerPrimitiveEvent event;
        event.id = exclusiveTransferPrimitiveID;
        event.value = exclusiveTransferPrimitiveMonotonicCounter;
        scheduler->Schedule(Queue::ExclusiveTransfer, syncBuffer, &event);
    }

    // Submissions always wait for the last mappings
    submitContext.waitPrimitives.Add(SchedulerPrimitiveEvent {
        .id = exclusiveTransferPrimitiveID,
        .value = exclusiveTransferPrimitiveMonotonicCounter
    });
    
    CommandBuilder postBuilder(submitContext.postContext->buffer);
    {
#if 0 // TODO[dbg]: Use or not?
        // Wait for any ongoing shaders
        builder.UAVBarrier();
        
        builder.SetShaderProgram(patchShaderProgramID);

        for (Breakpoint& breakpoint: breakpoints) {
            BreakpointPatchData patchData;
            patchData.allocationDWordOffset = static_cast<uint32_t>(breakpoint.allocation.offset / sizeof(uint32_t));
            patchData.streamDWordCount = static_cast<uint32_t>(breakpoint.allocation.length / sizeof(uint32_t)) - BreakpointStreamingHeaderDWordCount;

            //  TODO[dbg]: Indirect support is a must
            builder.SetDescriptorData(patchShaderProgram->GetPatchDataID(), patchData);
            builder.Dispatch((patchData.streamDWordCount + 255) / 256, 1, 1);
        }

        builder.UAVBarrier();
#endif

        // Copy the debug streaming buffer to host
        // TODO[dbg]: This is incorrect, of course
        for (const Breakpoint& breakpoint : breakpoints) {
            // TODO[dbg]: Now we're doing two copies, not so nice
            
            postBuilder.CopyBuffer(
                streamBufferID, breakpoint.uid * sizeof(BreakpointHeader),
                breakpoint.hostStreamingBuffer, 0,
                sizeof(BreakpointHeader)
            );
            
            postBuilder.CopyBuffer(
                streamBufferID, breakpoint.streamAllocation.offset,
                breakpoint.hostStreamingBuffer, sizeof(BreakpointHeader),
                breakpoint.streamAllocation.length
            );
        }
    }

    // Add to the tracker
    contextLifetimeQueue.Enqueue(submitContext, contexts, contextCount);
}

void DebugFeature::OnJoin(CommandContextHandle contextHandle) {
    std::lock_guard guard(mutex);
    contextLifetimeQueue.Join(contextHandle);
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

bool DebugFeature::CanCollectBreakpoint(const Breakpoint &breakpoint) {
    switch (breakpoint.captureMode) {
        default:
            ASSERT(false, "Invalid capture mode");
            return false;
        case BreakpointCaptureMode::FirstEvent:
            return breakpoint.pendingCollection;
        case BreakpointCaptureMode::AllEvents:
            return true;
    }
}

uint32_t DebugFeature::GetBreakpointInstrumentationHash(const Breakpoint &breakpoint, const BreakpointHeader *header) {
    switch (breakpoint.captureMode) {
        default:
            ASSERT(false, "Invalid capture mode");
            return 0u;
        case BreakpointCaptureMode::FirstEvent:
            return breakpoint.pendingCollectionHash;
        case BreakpointCaptureMode::AllEvents:
            return header->shaderInstrumentationHash32;
    }
}

bool DebugFeature::HasBreakpointStreambackData(const Breakpoint& breakpoint, const BreakpointHeader* header) {
    switch (breakpoint.captureMode) {
        default:
            ASSERT(false, "Invalid capture mode");
            return false;
        case BreakpointCaptureMode::FirstEvent:
            return true;
        case BreakpointCaptureMode::AllEvents:
            return header->dynamicCounter > 0;
    }
}

uint64_t DebugFeature::GetBreakpointStreamRequestSize(const Breakpoint &breakpoint, const BreakpointHeader *header, const BreakpointDataHostLayout& hostLayout) {
    switch (breakpoint.captureMode) {
        default:
            ASSERT(false, "Invalid capture mode");
            return 0;
        case BreakpointCaptureMode::FirstEvent:
            // Just stream back the entire thing
            return header->dwordStreamCount * sizeof(uint32_t);
        case BreakpointCaptureMode::AllEvents:
            // Stream back the actual contents
            return header->dynamicCounter * (sizeof(BreakpointLooseHeader) + hostLayout.dataDWordStride * sizeof(uint32_t));
    }
}

void DebugFeature::OnSyncPoint() {
    std::lock_guard guard(mutex);

    // No breakpoints? No data
    if (breakpoints.empty()) {
        return;
    }

    // Remove dead allocations
    allocationDestructionQueue.erase(std::ranges::remove_if(allocationDestructionQueue, [&](const PendingDestruction& pending) {
        if (!contextLifetimeQueue.IsCommitted(pending.lastCommit)) {
            return false;
        }

        // No longer in use, free the memory
        buddyAllocator.Free(pending.allocation);

        // TODO[dbg]: Fix the bug!
        // shaderDataHost->Destroy(pending.hostStreamingBuffer);
        return true;
    }).begin(), allocationDestructionQueue.end());

    // Throttle the data requests
    if (!ThrottleController(defaultController)) {
        return;
    }

    // Buffer for stages
    CommandBuffer buffer;
    CommandBuilder builder(buffer);
    
    // Stream out the breakpoints separately
    for (Breakpoint& breakpoint : breakpoints) {
        // Pending collection?
        if (!CanCollectBreakpoint(breakpoint)) {
            continue;
        }
        
        // Map the streaming buffer
        void* mapped = shaderDataHost->Map(breakpoint.hostStreamingBuffer);

        // Payload is after the header
        auto* header  = static_cast<BreakpointHeader*>(mapped);
        void* payload = header + 1;

        // Do we have any data at all?
        if (HasBreakpointStreambackData(breakpoint, header)) {
            // Get the instrumentation hash, this makes sure the host layout is always in sync
            uint32_t instrumentationHash32 = GetBreakpointInstrumentationHash(breakpoint, header);

            // Make sure it's a valid layout
            auto hostLayoutIt = breakpoint.hostLayoutMap.find(instrumentationHash32);
            if (hostLayoutIt == breakpoint.hostLayoutMap.end()) {
                continue;
            }

            // Describes the expected memory layout
            BreakpointDataHostLayout& hostLayout = hostLayoutIt->second;
            
            // How much we actually need to stream
            uint64_t requestedStreamSize = GetBreakpointStreamRequestSize(breakpoint, header, hostLayout);
            uint64_t effectiveStreamSize = std::min(breakpoint.streamSize, requestedStreamSize);
        
            // Empty out last stream
            MessageStreamView<DebugBreakpointStreamMessage> view(stream);

            // Allocate breakpoint data
            auto message = view.Add(DebugBreakpointStreamMessage::AllocationInfo {
                .dataCount = effectiveStreamSize,
                .dataTinyTypeCount = hostLayout.tinyType.size()
            });

            // TODO[dbg]: Can we somehow map this in-place? There's a lot of copies going on
            std::memcpy(message->data.Get(), payload, effectiveStreamSize);

            // Copy over tiny type
            std::memcpy(message->dataTinyType.Get(), hostLayout.tinyType.data(), hostLayout.tinyType.size());

            // Write out request data
            message->request = ++defaultController.requestIndex;
            message->uid = breakpoint.uid;
            message->dataFormat = static_cast<uint32_t>(hostLayout.format);
            message->dataTypeId = hostLayout.typeId;
            message->dataCompression = static_cast<uint32_t>(hostLayout.compression);
            message->dataDWordStride = hostLayout.dataDWordStride;
            message->dataOrder = static_cast<uint32_t>(header->dataOrder);
            message->dataStaticWidth = header->staticWidth;
            message->dataStaticHeight = header->staticHeight;
            message->dataStaticDepth = header->staticDepth;
            message->dataDynamicCounter = header->dynamicCounter;
            message->dataRequestStreamSize = static_cast<uint32_t>(requestedStreamSize);
        }

        // Done!
        shaderDataHost->Unmap(breakpoint.hostStreamingBuffer, mapped);
    
        // Patch the header
        builder.StageBuffer(streamBufferID, breakpoint.uid * sizeof(BreakpointHeader), sizeof(BreakpointHeader), &breakpoint.header);

        // Collected!
        breakpoint.pendingCollection = false;
    }

    // Any commands?
    if (buffer.Count()) {
        scheduler->Schedule(Queue::ExclusiveTransfer, buffer, nullptr);
    }
    
    // Any immediate streams?
    if (!stream.IsEmpty()) {
        bridge->GetOutput()->AddStreamAndSwap(stream);
    }
}

void DebugFeature::OnBreakpointAcquired(const BreakpointAcquisitionMessage *acqMessage, CommandBuilder& builder) {
    ASSERT(acqMessage->magic == 42, "Corrupt message");
    
    // If it failed to resolve, it may have been removed
    if (Breakpoint *breakpoint = FindBreakpointNoLock(acqMessage->uid)) {
        ASSERT(!breakpoint->pendingCollection, "GPU double-signalled breakpoint for collection");
        breakpoint->pendingCollection = true;
        
        // Read beyond primary key
        // TODO[init]: Add support for reading chunks in C++
        breakpoint->pendingCollectionHash = *(reinterpret_cast<const uint32_t*>(acqMessage) + 1);
    }
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

static bool IsTypeSerializationSupported(const Backend::IL::Type* type) {
    switch (type->kind) {
        default: {
            // TODO[dbg]: Matrix, Array
            // TODO[dbg]: Resources PRMT!
            return false;
        }
        case Backend::IL::TypeKind::Bool:
        case Backend::IL::TypeKind::Int:
        case Backend::IL::TypeKind::FP: {
            // Always supported
            return true;
        }
        case Backend::IL::TypeKind::Vector: {
            auto typed = type->As<Backend::IL::VectorType>();

            // Supports serialization if the contained can be
            return IsTypeSerializationSupported(typed->containedType);
        }
        case Backend::IL::TypeKind::Struct: {
            auto typed = type->As<Backend::IL::StructType>();

            // Supports serialization if every member can be
            for (const Backend::IL::Type *member: typed->memberTypes) {
                if (!IsTypeSerializationSupported(member)) {
                    return false;
                }
            }

            return true;
        }
    }
}

static bool SupportsImageFPUnormCompression(const IL::Instruction* instr, const Backend::IL::Type* type) {
    switch (type->kind) {
        default: {
            // Nope
            return false;
        }
        case Backend::IL::TypeKind::FP: {
            // Always supported
            return true;
        }
        case Backend::IL::TypeKind::Vector: {
            auto typed = type->As<Backend::IL::VectorType>();

            // Supports compression if the contained can be
            // Up to 4 components
            return SupportsImageFPUnormCompression(instr, typed->containedType) && typed->dimension <= 4u;
        }
        case Backend::IL::TypeKind::Struct: {
            auto typed = type->As<Backend::IL::StructType>();

            // Only supported for special case loads/samples
            if (!instr->Is<IL::LoadBufferInstruction>() &&
                !instr->Is<IL::LoadTextureInstruction>() &&
                !instr->Is<IL::SampleTextureInstruction>()) {
                return false;
            }

            // Supports compression if every member can be
            for (const Backend::IL::Type *member: typed->memberTypes) {
                if (!SupportsImageFPUnormCompression(instr, member)) {
                    return false;
                }
            }

            return true;
        }
    }
}

static IL::ID CompressFPUnorm8888(const IL::VisitContext &context, IL::Emitter<> emitter, IL::ID value) {
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
                IL::ID component = CompressFPUnorm8888(context, emitter, emitter.Extract(value, emitter.UInt32(i)));
                pixel = emitter.BitOr(pixel, emitter.BitShiftLeft(component, emitter.UInt32(i * 8)));
            }
            return pixel;
        }
        case Backend::IL::TypeKind::Struct: {
            auto* typed = type->As<Backend::IL::StructType>();

            // Compose color byte by byte
            IL::ID pixel = emitter.UInt32(0);
            for (uint32_t i = 0; i < typed->memberTypes.size(); i++) {
                IL::ID component = CompressFPUnorm8888(context, emitter, emitter.Extract(value, emitter.UInt32(i)));
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

bool DebugFeature::GetBreakpointFormat(const IL::VisitContext& context, const IL::Instruction* instr, IL::ID id, BreakpointData& breakpointData) {
    // Check compression
    switch (breakpointData.hostLayout.compression) {
        default: {
            //  No compression
            break;
        }
        case BreakpointCompression::FPUnorm8888: {
            breakpointData.hostLayout.format = Backend::IL::Format::RGBA8;
            return true;
        }
    }

    // No format
    // TODO[dbg]: This is obviously incomplete
    return false;
}

bool DebugFeature::SupportsKernelType(IL::KernelType kernelType, Breakpoint* breakpoint) {
    switch (breakpoint->captureMode) {
        default: {
            ASSERT(false, "Invalid mode");
            return false;
        }
        case BreakpointCaptureMode::FirstEvent: {
            // Only some shaders supported for now
            switch (kernelType) {
                default: {
                    ASSERT(false, "Invalid mode");
                    return false;
                }
                case IL::KernelType::None:
                case IL::KernelType::Vertex:
                case IL::KernelType::Geometry:
                case IL::KernelType::Hull:
                case IL::KernelType::Domain:
                case IL::KernelType::Amplification:
                case IL::KernelType::Mesh:
                case IL::KernelType::RayGen:
                case IL::KernelType::RayMiss:
                case IL::KernelType::RayHit:
                case IL::KernelType::Lib: {
                    // Not supported, yet
                    return false;
                }
                case IL::KernelType::Compute:
                case IL::KernelType::Pixel: {
                    // Supported
                    return true;
                }
            }
        }
        case BreakpointCaptureMode::AllEvents: {
            // Always supported
            return true;
        }
    }
}

bool DebugFeature::GetBreakpointDataHostLayout(const IL::VisitContext &context, const IL::Instruction* instr, IL::ID value, Breakpoint* breakpoint, BreakpointData& breakpointData) {
    // May not have an associated type
    const Backend::IL::Type *type = context.program.GetTypeMap().GetType(value);
    if (!type) {
        return false;
    }

    // Type may not be serializable
    if (!IsTypeSerializationSupported(type)) {
        return false;
    }

    // Check kernel type support
    auto* kernelType = context.program.GetMetadataMap().GetMetadata<IL::KernelTypeMetadata>(context.function.GetID());
    if (!SupportsKernelType(kernelType->type, breakpoint)) {
        return false;
    }

    // Supports 8-8-8-8 compression?
    if (breakpointData.flags & BreakpointFlag::AllowImageFPUNorm8888Compression && SupportsImageFPUnormCompression(instr, type)) {
        breakpointData.hostLayout.compression = BreakpointCompression::FPUnorm8888;
    }

    // Try to get the format
    if (GetBreakpointFormat(context, instr, value, breakpointData)) {
        // Assume stride
        breakpointData.hostLayout.dataDWordStride = static_cast<uint32_t>(GetSize(breakpointData.hostLayout.format) / sizeof(uint32_t));
    } else {
        // If not relevant, just assume the type
        breakpointData.hostLayout.typeId = type->id;

        // Pack the tiny type down
        Backend::IL::Tiny::Pack(type, breakpointData.hostLayout.tinyType);

        // TODO[dbg]: I guess we don't need to handle alignment?
        breakpointData.hostLayout.dataDWordStride = static_cast<uint32_t>((GetPODNonAlignedTypeByteSize(type) + sizeof(uint32_t) - 1) / sizeof(uint32_t));
    }

    // Host layout supported
    return true;
}

void DebugFeature::GetBreakpointOrderingFirstEvent(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ShaderStruct<ExecutionInfo>& execution, IL::ShaderBufferStruct<BreakpointHeader>& breakpointHeader, Breakpoint *breakpoint, BreakpointData& breakpointData) {
    auto* kernelType = context.program.GetMetadataMap().GetMetadata<IL::KernelTypeMetadata>(context.function.GetID());

    // Default init
    breakpointData.firstEvent.staticOrderWidth = emitter.UInt32(0);
    breakpointData.firstEvent.staticOrderHeight = emitter.UInt32(0);
    breakpointData.firstEvent.staticOrderDepth = emitter.UInt32(0);

    IL::ID dwordStride = emitter.UInt32(breakpointData.hostLayout.dataDWordStride);

    // Get work dimensions
    switch (kernelType->type) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case IL::KernelType::Pixel: {
            // Determine the thread counts
            breakpointData.firstEvent.staticOrderWidth = execution.Get<&ExecutionInfo::viewport>(emitter, 0);
            breakpointData.firstEvent.staticOrderHeight = execution.Get<&ExecutionInfo::viewport>(emitter, 1);
            breakpointData.firstEvent.staticOrderDepth = emitter.UInt32(1);
            break;
        }
        case IL::KernelType::Compute: {
            auto* kernelWorkgroupSize = context.program.GetMetadataMap().GetMetadata<IL::KernelWorkgroupSizeMetadata>(context.function.GetID());

            // Get the number of thread groups
            IL::ID threadGroupsX = execution.Get<&ExecutionInfo::dispatch>(emitter, 0);
            IL::ID threadGroupsY = execution.Get<&ExecutionInfo::dispatch>(emitter, 1);
            IL::ID threadGroupsZ = execution.Get<&ExecutionInfo::dispatch>(emitter, 2);

            // Determine the thread counts
            breakpointData.firstEvent.staticOrderWidth = emitter.Mul(threadGroupsX, emitter.UInt32(kernelWorkgroupSize->threadsX));
            breakpointData.firstEvent.staticOrderHeight = emitter.Mul(threadGroupsY, emitter.UInt32(kernelWorkgroupSize->threadsY));
            breakpointData.firstEvent.staticOrderDepth = emitter.Mul(threadGroupsZ, emitter.UInt32(kernelWorkgroupSize->threadsZ));
            break;
        }
    }

    // Determine the max number of dwords
    breakpointData.firstEvent.dwordStreamCount = emitter.Mul(breakpointData.firstEvent.staticOrderWidth, emitter.Mul(breakpointData.firstEvent.staticOrderHeight, emitter.Mul(breakpointData.firstEvent.staticOrderDepth, dwordStride)));

    // Max number of dwords
    IL::ID payloadDWordCount = breakpointHeader.Get<&BreakpointHeader::payloadDWordCount>(emitter);

    /// Is this a dynamic payload?
    breakpointData.firstEvent.isDynamic = emitter.GreaterThan(breakpointData.firstEvent.dwordStreamCount, payloadDWordCount);

    // Select dynamic if we exceed 
    breakpointData.orderType = emitter.Select(
        breakpointData.firstEvent.isDynamic,
        emitter.UInt32(static_cast<uint32_t>(BreakpointDataOrder::Dynamic)),
        emitter.UInt32(static_cast<uint32_t>(BreakpointDataOrder::Static))
    );

    // Indices
    IL::ID x = IL::InvalidID;
    IL::ID y = IL::InvalidID;
    IL::ID z = IL::InvalidID;

    // Get thread indices
    switch (kernelType->type) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case IL::KernelType::Pixel: {
            // Get typed dispatch index
            IL::ID pos = emitter.KernelValue(Backend::IL::KernelValue::PixelPosition);

            // Get dimensions
            x = emitter.FloatToUInt32(emitter.Extract(pos, emitter.UInt32(0)));
            y = emitter.FloatToUInt32(emitter.Extract(pos, emitter.UInt32(1)));
            z = emitter.UInt32(0);
            break;
        }
        case IL::KernelType::Compute: {
            // Get typed dispatch index
            IL::ID dtid = emitter.KernelValue(Backend::IL::KernelValue::DispatchThreadID);

            // Get dimensions
            x = emitter.Extract(dtid, emitter.UInt32(0));
            y = emitter.Extract(dtid, emitter.UInt32(1));
            z = emitter.Extract(dtid, emitter.UInt32(2));
            break;
        }
    }

    // Static ordering
    IL::ID staticOrder;
    {
        // z * w * h + y * w + x
        staticOrder = emitter.Mul(z, emitter.Mul(breakpointData.firstEvent.staticOrderWidth, breakpointData.firstEvent.staticOrderHeight));
        staticOrder = emitter.Add(staticOrder, emitter.Mul(y, breakpointData.firstEvent.staticOrderWidth));
        staticOrder = emitter.Add(staticOrder, x);
    }

    // Assume static ordering for now, dynamic exporting happens later
    breakpointData.staticOrder = staticOrder;
    
#if !defined(NDEBUG) && 0
    breakpointHeader.Set<&BreakpointHeader::debugPayloads>(emitter, x, 0);
    breakpointHeader.Set<&BreakpointHeader::debugPayloads>(emitter, y, 1);
    breakpointHeader.Set<&BreakpointHeader::debugPayloads>(emitter, z, 2);
#endif // NDEBUG
}

static void GetBreakpointDataDWords(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ID value, uint32_t& byteOffset, TrivialStackVector<IL::ID, 16u>& dwords) {
    const Backend::IL::Type *type = context.program.GetTypeMap().GetType(value);

    // Structural
    switch (type->kind) {
        default: {
            break;
        }
        case Backend::IL::TypeKind::Vector: {
            auto typed = type->As<Backend::IL::VectorType>();

            // Get all nested dwords
            for (uint32_t i = 0; i < typed->dimension; i++) {
                GetBreakpointDataDWords(
                    context, emitter, emitter.Extract(value, emitter.UInt32(i)),
                    byteOffset,
                    dwords
                );
            }
            
            return;
        }
        case Backend::IL::TypeKind::Struct: {
            auto typed = type->As<Backend::IL::StructType>();

            // Get all nested dwords
            for (uint32_t i = 0; i < static_cast<uint32_t>(typed->memberTypes.size()); i++) {
                GetBreakpointDataDWords(
                    context, emitter, emitter.Extract(value, emitter.UInt32(i)),
                    byteOffset,
                    dwords
                );
            }
            
            return;
        }
    }

    // TODO: Hmm, >32 bit handling?
    const Backend::IL::Type *encodeType = context.program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType {
        .bitWidth = 32
    });
    
    // Type conversion
    switch (type->kind) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case Backend::IL::TypeKind::Bool: {
            value = emitter.Select(value, emitter.UInt32(1), emitter.UInt32(0));
            break;
        }
        case Backend::IL::TypeKind::Int:
        case Backend::IL::TypeKind::FP: {
            // TODO[dbg]: Handle ext
            // TODO[dbg]: double?
            break;
        }
    }

    // Cast to u32
    value = emitter.BitCast(value, encodeType);

    // Get the number of bytes
    uint32_t byteCount = static_cast<uint32_t>(GetPODNonAlignedTypeByteSize(type));

    // Fast path for dword aligned offset and data
    if (byteOffset % sizeof(uint32_t) == 0 && byteCount == sizeof(uint32_t)) {
        uint32_t dword = static_cast<uint32_t>(byteOffset / sizeof(uint32_t));
        dwords[dword] = value;
        byteOffset += sizeof(uint32_t);
        return;
    }

    // Not aligned, write the data out, carefully handling the dword boundaries
    for (uint32_t componentByteOffset = 0; componentByteOffset < byteCount;) {
        uint32_t dword          = static_cast<uint32_t>(byteOffset / sizeof(uint32_t));
        uint32_t dwordBitOffset = static_cast<uint32_t>((byteOffset % sizeof(uint32_t)) * 8);
        uint32_t nextDWord      = dword + 1;
        uint32_t remBytes       = std::min(byteCount - componentByteOffset, static_cast<uint32_t>(nextDWord * sizeof(uint32_t)) - componentByteOffset);
        
        // Take the value, cut out the portion that we're writing
        IL::ID maskedValue = emitter.BitAnd(
            emitter.BitShiftRight(value, emitter.UInt32(static_cast<uint32_t>(componentByteOffset % sizeof(uint32_t)) * 8)),
            emitter.UInt32(std::bit_width(remBytes) - 1)
        );

        // Write the destination dword
        dwords[dword] = emitter.BitOr(dwords[dword], emitter.BitShiftLeft(maskedValue, emitter.UInt32(dwordBitOffset)));

        // Next offset
        componentByteOffset += remBytes;
        byteOffset += remBytes;
    }
}

void DebugFeature::StoreBreakpointDataDWords(const IL::VisitContext &context, IL::Emitter<>& emitter, IL::ID value, Breakpoint* breakpoint, BreakpointData& breakpointData) {
    // Zero init dwords
    TrivialStackVector<IL::ID, 16u> dwords(breakpointData.hostLayout.dataDWordStride);
    for (uint32_t i = 0; i < breakpointData.hostLayout.dataDWordStride; i++) {
        dwords[i] = emitter.UInt32(0);
    }

    // Get the data dwords, handles alignment
    uint32_t byteOffset = 0;
    GetBreakpointDataDWords(context, emitter, value, byteOffset, dwords);

    // Get the data ids
    IL::ID streamLoadID = emitter.Load(context.program.GetShaderDataMap().Get(streamBufferID)->id);
    
    // Finally, write them out
    for (uint32_t i = 0; i < breakpointData.hostLayout.dataDWordStride; i++) {
        IL::ID offset = emitter.Add(breakpointData.payloadDataOffset, emitter.UInt32(i));
        emitter.StoreBuffer(streamLoadID, offset, dwords[i]);
    }
}

void DebugFeature::StoreBreakpointData(const IL::VisitContext &context, IL::Emitter<>& emitter, const IL::Instruction* instr, IL::ID value, Breakpoint* breakpoint, BreakpointData& breakpointData) {    
    // Handle any kind of compression
    switch (breakpointData.hostLayout.compression) {
        default: {
            //  No compression
            break;
        }
        case BreakpointCompression::FPUnorm8888: {
            // Compress the value
            value = CompressFPUnorm8888(context, emitter, value);

            // 255 Alpha
            // TODO[dbg]: Remove this, testing only
            value = emitter.BitOr(value, emitter.UInt32(0xFF << 24));
            break;
        }
    }

    // Finally, store the dwords
    StoreBreakpointDataDWords(context, emitter, value, breakpoint, breakpointData);
}

static uint32_t ShaderInstrumentationHashWideTo32(uint64_t wide) {
    return BufferCRC32Short(&wide, sizeof(wide));
}

IL::BasicBlock::Iterator DebugFeature::InjectBreakpoint(const IL::VisitContext &context, const IL::BasicBlock::Iterator &it, const DebugBreakpointMessage& breakpointMessage) {
    // TODO[dbg]: Send a message back "nothing to debug!" This shouldn't come from a message

    // Find the relevant breakpoint
    Breakpoint* breakpoint = FindBreakpointNoLock(breakpointMessage.uid);
    if (!breakpoint) {
        return it;
    }
    
    // Get the value to be debugged
    IL::ID value = GetInstructionDebugValue(it);
    if (value == IL::InvalidID) {
        return it;
    }

    // Intermediate data
    BreakpointData breakpointData;
    breakpointData.flags = static_cast<BreakpointFlag>(breakpointMessage.flags);
    breakpointData.shaderInstrumentationHash32 = ShaderInstrumentationHashWideTo32(context.program.GetShaderInstrumentationHash());

    // Try to determine the data layout
    // This may fail if there's nothing suitable
    if (!GetBreakpointDataHostLayout(context, it, value, breakpoint, breakpointData)) {
        return it;
    }

    /**
     * At this point the breakpoint has been accepted, emitting is allowed
     **/
    
    // Emit in the interrupt block
    IL::BasicBlock* interruptBlock = context.function.GetBasicBlocks().AllocBlock();
    IL::Emitter<>   emitter(context.program, *interruptBlock);

    // Acquire it logically before, to access some shared findings
    IL::BasicBlock* resumeBlock;
    switch (breakpoint->captureMode) {
        default:
            ASSERT(false, "Invalid capture mode");
            return it;
        case BreakpointCaptureMode::FirstEvent:
            resumeBlock = AcquireAndAllocateBreakpointFirstEvent(context, it, interruptBlock, breakpoint, breakpointData);
            break;
        case BreakpointCaptureMode::AllEvents:
            resumeBlock = AcquireAndAllocateBreakpointAllEvents(context, it, interruptBlock, breakpoint, breakpointData);
            break;
    }

    // Store the breakpoint data
    StoreBreakpointData(context, emitter, resumeBlock->begin(), value, breakpoint, breakpointData);
    
    // Branch the breakpoint to resume
    IL::Emitter(context.program, *interruptBlock).Branch(resumeBlock);

    // Instrumentation has passed, keep the layout around
    breakpoint->hostLayoutMap[breakpointData.shaderInstrumentationHash32] = breakpointData.hostLayout;

    // Resume iteration
    return resumeBlock->begin();
}

IL::BasicBlock* DebugFeature::AcquireBreakpointFirstEvent(const IL::VisitContext &context, const IL::BasicBlock::Iterator &it, IL::BasicBlock *breakpointBlock, Breakpoint* breakpoint, BreakpointData& breakpointData) {
    /**
     * First-event optimized acquisition.
     *
     * acq = (header.acquiredExecutionUID == rollingExecutionUID)
     * if (header.acquiredExecutionUID == 0) {
     *   last = AtomicCAS(&header.acquiredExecutionUID, 0, rollingExecutionUID)
     *   
     *   allocated = (last == 0)
     *   if (allocated) {
     *     Export(BreakpointAcquisitionMessage {
     *       .uid = <uid>
     *     });
     *   }
     *   
     *   acq = (allocated || last == rollingExecutionUID)
     * }
     *
     * acq = phi <...>
     */

    // Allocate blocks
    IL::BasicBlock* headerBlock      = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.Header");
    IL::BasicBlock* casBlock         = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.CAS");
    IL::BasicBlock* casMerge         = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.CAS.Merge");
    IL::BasicBlock* resumeBlock      = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.Resume");
    IL::BasicBlock* exportBlock      = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.Export");
    IL::BasicBlock* exportMergeBlock = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.Export.Merge");
    
    // Split the iterator to resume
    // Excluding the iterator itself, since we may want to reference the instruction results
    it.block->Split(resumeBlock, std::next(it));

    // Immediately branch to the header
    IL::Emitter(context.program, *it.block).Branch(headerBlock);
    IL::Emitter<> headerEmitter(context.program, *headerBlock);

    // Find the relevant breakpoint
    GetBreakpoint(headerEmitter, breakpoint, breakpointData);

    // Get the current execution
    IL::ShaderStruct<ExecutionInfo> execution(headerEmitter.ExecutionInfo());

    // Get the header
    IL::ShaderBufferStruct<BreakpointHeader> breakpointHeader(context.program.GetShaderDataMap().Get(streamBufferID)->id, breakpointData.headerOffset);

    // Get payload offset
    breakpointData.payloadOffset = breakpointHeader.Get<&BreakpointHeader::payloadDWordOffset>(headerEmitter);

    // Get the ordering, this is used by both the acquire header and export
    GetBreakpointOrderingFirstEvent(context, headerEmitter, execution, breakpointHeader, breakpoint, breakpointData);

    // Read the curent acquired UID
    // This is a regular buffer read, not atomic
    IL::ID acquiredUID = breakpointHeader.Get<&BreakpointHeader::acquiredExecutionUID>(headerEmitter);

    // Acquired states, first check if it's equal to the current UID
    IL::ID executionUID      = execution.Get<&ExecutionInfo::rollingExecutionUID>(headerEmitter);
    IL::ID acquiredHeader = headerEmitter.Equal(acquiredUID, executionUID);
    IL::ID acquiredCAS       = IL::InvalidID;

    // If not acquired, and it's equal to zero (i.e., unallocated), enter the CAS block
    // With this we've validated that we don't hold the lock and nothing else does.
    // Of course, the cache lines may not represent the real state, but in case of mismatches
    // we'll enter the CAS block anyhow.
    headerEmitter.BranchConditional(
        headerEmitter.Equal(acquiredUID, headerEmitter.UInt32(0)),
        casBlock,
        casMerge,
        IL::ControlFlow::Selection(casMerge)
    );

    // CAS
    {
        IL::Emitter<> casEmitter(context.program, *casBlock);

        // Actually do the CAS
        IL::ID previousValue = breakpointHeader.AtomicCompareExchange<&BreakpointHeader::acquiredExecutionUID>(casEmitter, casEmitter.UInt32(0), executionUID);

        // Allocated if it was zero
        IL::ID allocatedCAS = casEmitter.Equal(previousValue, casEmitter.UInt32(0));

        // Either we allocated or acquired the UID
        acquiredCAS = casEmitter.Or(allocatedCAS, casEmitter.Equal(previousValue, executionUID));

        // If allocated, move to the export block
        casEmitter.BranchConditional(allocatedCAS, exportBlock, exportMergeBlock, IL::ControlFlow::Selection(exportMergeBlock));

        // Export
        {
            IL::Emitter<> exportEmitter(context.program, *exportBlock);

            // Write out that the breakpoint was allocated
            // Since streams are per-submission, this is entirely atomic and coherent
            BreakpointAcquisitionMessage::ShaderExport msg;
            msg.chunks |= BreakpointAcquisitionMessage::Chunk::ExtraData;
            msg.uid = exportEmitter.UInt32(breakpoint->uid);
            msg.magic = exportEmitter.UInt32(42);
            msg.extraData.instrumentationHash32 = exportEmitter.UInt32(breakpointData.shaderInstrumentationHash32);
            exportEmitter.Export(exportID, msg);

            // Update the breakpoint header's device data layout
            // Used on the host to resolve ordering
            breakpointHeader.Set<&BreakpointHeader::dataOrder>(exportEmitter, breakpointData.orderType);
            breakpointHeader.Set<&BreakpointHeader::staticWidth>(exportEmitter, breakpointData.firstEvent.staticOrderWidth);
            breakpointHeader.Set<&BreakpointHeader::staticHeight>(exportEmitter, breakpointData.firstEvent.staticOrderHeight);
            breakpointHeader.Set<&BreakpointHeader::staticDepth>(exportEmitter, breakpointData.firstEvent.staticOrderDepth);
            breakpointHeader.Set<&BreakpointHeader::dwordStreamCount>(exportEmitter, breakpointData.firstEvent.dwordStreamCount);

            exportEmitter.Branch(exportMergeBlock);
        }
        
        // Export Merge
        {
            IL::Emitter<> mergeEmitter(context.program, *exportMergeBlock);
            mergeEmitter.Branch(casMerge);
        }
    }

    // Merge
    {
        IL::Emitter<> mergeEmitter(context.program, *casMerge);

        // Merge the inbound acquired states
        IL::ID acquired = mergeEmitter.Phi(headerBlock, acquiredHeader, exportMergeBlock, acquiredCAS);

        // If acquired, do breakpoint stuff, otherwise resume the program as usual
        mergeEmitter.BranchConditional(
            acquired,
            breakpointBlock,
            resumeBlock,
            IL::ControlFlow::Selection(resumeBlock)
        );
    }

    // Iterate again
    return resumeBlock;
}

IL::BasicBlock * DebugFeature::AcquireAndAllocateBreakpointFirstEvent(const IL::VisitContext &context, const IL::BasicBlock::Iterator &it, IL::BasicBlock *breakpointBlock, Breakpoint *breakpoint, BreakpointData &breakpointData) {
    /**
    * acq = acquire()
    * if (acq) {
    *   if (dynamic) {
    *     order = allocDynamic()
    *   }
    *
    *   order = phi <...>
    *   breakpoint
    */

    // Allocate blocks
    IL::BasicBlock* headerBlock  = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.Header");
    IL::BasicBlock* dynamicBlock = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.DynamicAlloc");
    IL::BasicBlock* mergeBlock   = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.Merge");

    // Acquire the breakpoint
    IL::BasicBlock* resumeBlock = AcquireBreakpointFirstEvent(context, it, headerBlock, breakpoint, breakpointData);

    // Header
    IL::ID staticPayloadDataOffset;
    {
        IL::Emitter<> emitter(context.program, *headerBlock);

        // Header offset within the payload
        staticPayloadDataOffset = emitter.Mul(breakpointData.staticOrder, emitter.UInt32(breakpointData.hostLayout.dataDWordStride));

        // Offset by payload offset
        staticPayloadDataOffset = emitter.Add(breakpointData.payloadOffset, staticPayloadDataOffset);
        
        emitter.BranchConditional(breakpointData.firstEvent.isDynamic, dynamicBlock, mergeBlock, IL::ControlFlow::Selection(mergeBlock));
    }

    // Dynamic Allocation
    IL::ID dynamicOrder;
    IL::ID dynamicPayloadDataOffset;
    {
        IL::Emitter<> dynamicEmitter(context.program, *dynamicBlock);

        // Get the header
        IL::ShaderBufferStruct<BreakpointHeader> breakpointHeader(context.program.GetShaderDataMap().Get(streamBufferID)->id, breakpointData.headerOffset);

        // Get dynamic ordering
        dynamicOrder = breakpointHeader.AtomicAdd<&BreakpointHeader::dynamicCounter>(dynamicEmitter, dynamicEmitter.UInt32(1));

        // Limit by available number of dwords
        dynamicOrder = IL::ExtendedEmitter(dynamicEmitter).Min(dynamicOrder, breakpointHeader.Get<&BreakpointHeader::payloadDWordCount>(dynamicEmitter));

        // Header offset within the payload
        IL::ID dynamicHeaderDWordOffset = dynamicEmitter.Mul(dynamicOrder, dynamicEmitter.UInt32(breakpointData.hostLayout.dataDWordStride + BreakpointDynamicHeaderDWordCount));

        // Offset by payload offset
        dynamicHeaderDWordOffset = dynamicEmitter.Add(breakpointData.payloadOffset, dynamicHeaderDWordOffset);

        // Store the dynamic header
        IL::ID streamLoadID = dynamicEmitter.Load(context.program.GetShaderDataMap().Get(streamBufferID)->id);
        dynamicEmitter.StoreBuffer(streamLoadID, dynamicHeaderDWordOffset, breakpointData.staticOrder);

        // Start writing after the header
        dynamicPayloadDataOffset = dynamicEmitter.Add(dynamicHeaderDWordOffset, dynamicEmitter.UInt32(BreakpointDynamicHeaderDWordCount));

        // To merge
        dynamicEmitter.Branch(mergeBlock);
    }

    // Merge
    {
        IL::Emitter<> mergeEmitter(context.program, *mergeBlock);

        // Select the appropriate ordering
        breakpointData.exportOrder = mergeEmitter.Phi(
            headerBlock, breakpointData.staticOrder,
            dynamicBlock, dynamicOrder
        );

        // Select the data offset
        breakpointData.payloadDataOffset = mergeEmitter.Phi(
            headerBlock, staticPayloadDataOffset,
            dynamicBlock, dynamicPayloadDataOffset
        );

        // To the actual breakpoint
        mergeEmitter.Branch(breakpointBlock);
    }

    // OK
    return resumeBlock;
}

IL::BasicBlock * DebugFeature::AcquireAndAllocateBreakpointAllEvents(const IL::VisitContext &context, const IL::BasicBlock::Iterator &it, IL::BasicBlock *interruptBlock, Breakpoint *breakpoint, BreakpointData &breakpointData) {
    /**
     * <instr>
     *
     * if (!header.instrumentationHash32) {
     *   atomicCAS(header.instrumentationHash32, 0, <version>)
     * }
     *
     * if (header.instrumentationHash32 == <version>) {
     *   setup
     *   <interrupt>
     * }
     */

    // Allocate blocks
    IL::BasicBlock* hashHeaderBlock     = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.HashHeader");
    IL::BasicBlock* hashAllocationBlock = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.HashAllocation");
    IL::BasicBlock* hashMergeBlock      = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.HashMerge");
    IL::BasicBlock* setupBlock          = context.function.GetBasicBlocks().AllocBlock("Bk.Inject.Setup");
    IL::BasicBlock* resumeBlock         = context.function.GetBasicBlocks().AllocBlock("Bk.Acquire.Resume");

    // Split the iterator to resume
    // Excluding the iterator itself, since we may want to reference the instruction results
    it.block->Split(resumeBlock, std::next(it));
    
    // Immediately branch to the header
    IL::Emitter(context.program, *it.block).Branch(hashHeaderBlock);

    // Shared header
    IL::ShaderBufferStruct<BreakpointHeader> breakpointHeader;
    
    // Hash header
    // Check if we're allocated or not
    {
        IL::Emitter<> emitter(context.program, *hashHeaderBlock);

        // Find the relevant breakpoint
        GetBreakpoint(emitter, breakpoint, breakpointData);
        
        // Get the header
        breakpointHeader = IL::ShaderBufferStruct<BreakpointHeader>(context.program.GetShaderDataMap().Get(streamBufferID)->id, breakpointData.headerOffset);

        // Get payload offset
        breakpointData.payloadOffset = breakpointHeader.Get<&BreakpointHeader::payloadDWordOffset>(emitter);

        // Check if the hash is unallocated
        IL::ID isUnallocatedHash = emitter.Equal(
            breakpointHeader.Get<&BreakpointHeader::shaderInstrumentationHash32>(emitter),
            emitter.UInt32(0)
        );

        // Allocate if need be
        emitter.BranchConditional(isUnallocatedHash, hashAllocationBlock, hashMergeBlock, IL::ControlFlow::Selection(hashMergeBlock));
    }

    // Hash allocation
    // Try to allocate the current hash
    {
        IL::Emitter<> emitter(context.program, *hashAllocationBlock);
        
        // Actually do the CAS
        breakpointHeader.AtomicCompareExchange<&BreakpointHeader::shaderInstrumentationHash32>(emitter, emitter.UInt32(0), emitter.UInt32(breakpointData.shaderInstrumentationHash32));

        // Back to merge
        emitter.Branch(hashMergeBlock);
    }

    // Hash merge
    {
        IL::Emitter<> emitter(context.program, *hashMergeBlock);

        // Check if the hash is matching
        IL::ID isMatchingHash = emitter.Equal(
            breakpointHeader.Get<&BreakpointHeader::shaderInstrumentationHash32>(emitter),
            emitter.UInt32(breakpointData.shaderInstrumentationHash32)
        );

        // Allocate if need be
        emitter.BranchConditional(isMatchingHash, setupBlock, resumeBlock, IL::ControlFlow::Selection(resumeBlock));
    }

    // Setup
    {
        IL::Emitter<> emitter(context.program, *setupBlock);

        // Get dynamic ordering
        IL::ID order = breakpointHeader.AtomicAdd<&BreakpointHeader::dynamicCounter>(emitter, emitter.UInt32(1));

        // Limit by available number of dwords
        order = IL::ExtendedEmitter(emitter).Min(order, breakpointHeader.Get<&BreakpointHeader::payloadDWordCount>(emitter));

        // Header offset within the payload
        IL::ID headerDWordOffset = emitter.Mul(order, emitter.UInt32(breakpointData.hostLayout.dataDWordStride + BreakpointLooseHeaderDWordCount));

        // Offset by payload offset
        headerDWordOffset = emitter.Add(breakpointData.payloadOffset, headerDWordOffset);

        // Store the dynamic header
        IL::ID streamLoadID = emitter.Load(context.program.GetShaderDataMap().Get(streamBufferID)->id);

        // Write out the loose header
        {
            // Get the current execution
            IL::ShaderStruct<ExecutionInfo> execution(emitter.ExecutionInfo());

            // Copy over the full execution info
            for (uint32_t  i = 0; i < kExecutionInfoDWordCount; i++) {
                emitter.StoreBuffer(streamLoadID, emitter.Add(headerDWordOffset, emitter.UInt32(i)), execution.GetDWord(emitter, i));
            }

            // Local thread data
            IL::ID threadX = IL::InvalidID;
            IL::ID threadY = IL::InvalidID;
            IL::ID threadZ = IL::InvalidID;

            // Get the thread indices
            auto* kernelType = context.program.GetMetadataMap().GetMetadata<IL::KernelTypeMetadata>(context.program.GetEntryPoint()->GetID());
            switch (kernelType->type) {
                default: {
                    // Just default to zero for now
                    threadX = emitter.UInt32(0);
                    threadY = emitter.UInt32(0);
                    threadZ = emitter.UInt32(0);
                    break;
                }
                case IL::KernelType::Pixel: {
                    // Get typed dispatch index
                    IL::ID pos = emitter.KernelValue(Backend::IL::KernelValue::PixelPosition);

                    // Get dimensions
                    threadX = emitter.FloatToUInt32(emitter.Extract(pos, emitter.UInt32(0)));
                    threadY = emitter.FloatToUInt32(emitter.Extract(pos, emitter.UInt32(1)));
                    threadZ = emitter.UInt32(0);
                    break;
                }
                case IL::KernelType::Compute: {
                    IL::ID threadId = emitter.KernelValue(Backend::IL::KernelValue::DispatchThreadID);
                    threadX = emitter.Extract(threadId, emitter.UInt32(0));
                    threadY = emitter.Extract(threadId, emitter.UInt32(1));
                    threadZ = emitter.Extract(threadId, emitter.UInt32(2));
                    break;
                }
            }

            // Store the thread indices
            emitter.StoreBuffer(streamLoadID, emitter.Add(headerDWordOffset, emitter.UInt32(IL::MemberDWordOffset<&BreakpointLooseHeader::threadX>())), threadX);
            emitter.StoreBuffer(streamLoadID, emitter.Add(headerDWordOffset, emitter.UInt32(IL::MemberDWordOffset<&BreakpointLooseHeader::threadY>())), threadY);
            emitter.StoreBuffer(streamLoadID, emitter.Add(headerDWordOffset, emitter.UInt32(IL::MemberDWordOffset<&BreakpointLooseHeader::threadZ>())), threadZ);
        }

        // Start writing after the header
        breakpointData.payloadDataOffset = emitter.Add(headerDWordOffset, emitter.UInt32(BreakpointLooseHeaderDWordCount));
        breakpointData.exportOrder = order;

        // To merge
        emitter.Branch(interruptBlock);
    }

    // OK
    return resumeBlock;
}

void DebugFeature::CreateAndUpdatePayload(Breakpoint &breakpoint) {
    // Allocate the underlying memory
    breakpoint.streamAllocation = buddyAllocator.Allocate(breakpoint.streamSize);

    // Setup the default header
    breakpoint.header.payloadDWordOffset = static_cast<uint32_t>(breakpoint.streamAllocation.offset / sizeof(uint32_t));
    breakpoint.header.payloadDWordCount  = static_cast<uint32_t>(breakpoint.streamSize / sizeof(uint32_t));

    // Capture mode modifiers
    switch (breakpoint.captureMode) {
        default:
            break;
        case BreakpointCaptureMode::AllEvents:
            breakpoint.header.dataOrder = BreakpointDataOrder::Loose;
            break;
    }

    // Map the relevant times for the range
    tileResidencyAllocator.Allocate(
        breakpoint.streamAllocation.offset,
        breakpoint.streamAllocation.length
    );

    // Create streaming counter-part
    breakpoint.hostStreamingBuffer = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = breakpoint.streamAllocation.length + sizeof(BreakpointHeader),
        .format = Backend::IL::Format::R8UInt,
        .flagSet = ShaderDataBufferFlag::Host
    }, "DebugStreamHost");
}

DebugFeature::Breakpoint * DebugFeature::FindBreakpointNoLock(uint32_t uid) {
    // Find the relevant breakpoint
    for (Breakpoint& _candidate : breakpoints) {
        if (_candidate.uid == uid) {
            return &_candidate;
        }
    }

    // Not found
    return nullptr;
}

void DebugFeature::GetBreakpoint(IL::Emitter<> &emitter, Breakpoint *breakpoint, BreakpointData& breakpointData) {
    // Set the dword offset
    breakpointData.headerOffset = emitter.UInt32(breakpoint->uid * BreakpointHeaderDWordCount);
}

void DebugFeature::GetBreakpoint(IL::Emitter<> &emitter, DebugBreakpointMessage breakpoint, BreakpointData& breakpointData) {
    std::lock_guard guard(mutex);

    // Find the relevant breakpoint
    Breakpoint* candidate = FindBreakpointNoLock(breakpoint.uid);

    // Shouldn't happen
    if (!candidate) {
        ASSERT(false, "Invalid candidate");
        return;
    }

    // Set the dword offset
    GetBreakpoint(emitter, candidate, breakpointData);
}

FeatureInfo DebugFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Debug";
    info.description = "";
    return info;
}
