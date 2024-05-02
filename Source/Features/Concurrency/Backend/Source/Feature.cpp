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

#include <Features/Concurrency/Feature.h>
#include <Features/Descriptor/Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/Emitters/ResourceTokenEmitter.h>
#include <Backend/CommandContext.h>
#include <Backend/SubmissionContext.h>
#include <Backend/Scheduler/IScheduler.h>
#include <Backend/Scheduler/SchedulerPrimitiveEvent.h>

// Addressing
#include <Addressing/TexelMemoryAllocator.h>
#include <Addressing/IL/BitIndexing.h>
#include <Addressing/IL/Emitters/TexelPropertiesEmitter.h>

// Generated schema
#include <Schemas/Features/Concurrency.h>
#include <Schemas/Instrumentation.h>
#include <Schemas/InstrumentationCommon.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Common
#include <Common/Registry.h>

bool ConcurrencyFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<ResourceRaceConditionMessage>();

    // Optional SGUID host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Get scheduler
    scheduler = registry->Get<IScheduler>();

    // Create monotonic primitive
    exclusiveTransferPrimitiveID = scheduler->CreatePrimitive();

    // Allocate puid mapping buffer
    puidMemoryBaseBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = 1u << Backend::IL::kResourceTokenPUIDBitCount,
        .format = Backend::IL::Format::R32UInt
    });

    // Try to install texel allocator
    texelAllocator = registry->New<TexelMemoryAllocator>();
    if (!texelAllocator->Install()) {
        return false;
    }

    // OK
    return true;
}

FeatureHookTable ConcurrencyFeature::GetHookTable() {
    FeatureHookTable table{};
    table.createResource = BindDelegate(this, ConcurrencyFeature::OnCreateResource);
    table.destroyResource = BindDelegate(this, ConcurrencyFeature::OnDestroyResource);
    table.preSubmit = BindDelegate(this, ConcurrencyFeature::OnSubmitBatchBegin);
    return table;
}

void ConcurrencyFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void ConcurrencyFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void ConcurrencyFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const SetInstrumentationConfigMessage config = CollapseOrDefault<SetInstrumentationConfigMessage>(specialization);
    
    // Get the data ids
    IL::ID puidMemoryBaseBufferDataID = program.GetShaderDataMap().Get(puidMemoryBaseBufferID)->id;
    IL::ID texelMaskBufferDataID      = program.GetShaderDataMap().Get(texelAllocator->GetTexelBlocksBufferID())->id;

    // Common constants
    IL::ID zero = program.GetConstants().UInt(0)->id;

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Is write operation?
        bool isWrite = false;

        // Instruction of interest?
        IL::ID resource;
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::LoadBuffer: {
                resource = it->As<IL::LoadBufferInstruction>()->buffer;
                break;
            }
            case IL::OpCode::StoreBuffer: {
                resource = it->As<IL::StoreBufferInstruction>()->buffer;
                isWrite = true;
                break;
            }
            case IL::OpCode::LoadBufferRaw: {
                resource = it->As<IL::LoadBufferRawInstruction>()->buffer;
                break;
            }
            case IL::OpCode::StoreBufferRaw: {
                resource = it->As<IL::StoreBufferRawInstruction>()->buffer;
                isWrite = true;
                break;
            }
            case IL::OpCode::StoreTexture: {
                resource = it->As<IL::StoreTextureInstruction>()->texture;
                isWrite = true;
                break;
            }
            case IL::OpCode::LoadTexture: {
                resource = it->As<IL::LoadTextureInstruction>()->texture;

                // Get type
                auto type = program.GetTypeMap().GetType(resource)->As<Backend::IL::TextureType>();

                // Sub-pass inputs are not validated
                if (type->dimension == Backend::IL::TextureDimension::SubPass) {
                    return it;
                }
                break;
            }
            case IL::OpCode::SampleTexture: {
                resource = it->As<IL::SampleTextureInstruction>()->texture;
                break;
            }
        }

        // Bind the SGUID
        ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

        // Allocate blocks
        IL::BasicBlock* resumeBlock = context.function.GetBasicBlocks().AllocBlock();
        IL::BasicBlock* oobBlock    = context.function.GetBasicBlocks().AllocBlock();

        // Split this basic block, move all instructions post and including the instrumented instruction to resume
        // ! iterator invalidated
        auto instr = context.basicBlock.Split(resumeBlock, it);

        // Perform instrumentation check
        IL::Emitter<> pre(program, context.basicBlock);

        // Get the texel address
        Backend::IL::TexelPropertiesEmitter propertiesEmitter(pre, texelAllocator, puidMemoryBaseBufferDataID);
        TexelProperties texelProperties = propertiesEmitter.GetTexelProperties(IL::InstructionRef(instr));

        // Manually select the target bit, this follows the same mechanism as the other overloads,
        // in our case we set the target bit to 0 if the address is out of bounds. Effectively disabling
        // the atomic writes without adding block branching logic.
        IL::ID texelBlockBit = pre.Select(
            texelProperties.address.isOutOfBounds,
            program.GetConstants().UInt(0)->id,
            GetTexelAddressBit(pre, texelProperties.address.texelOffset)
        );
        
        // Read the previous lock, semantics change if write
        IL::ID previousLock;
        if (isWrite) {
            // Writes follow the single producer pattern, so, write the destination bit and check if it was already locked
            // Basically an atomic or with the target bit
            previousLock = AtomicOrTexelAddress(pre, texelMaskBufferDataID, texelProperties.texelBaseOffsetAlign32, texelProperties.address.texelOffset, texelBlockBit);
        } else {
            // Reads follow multiple-consumers, so check if anything has locked the destination bit
            previousLock = ReadTexelAddress(pre, texelMaskBufferDataID, texelProperties.texelBaseOffsetAlign32, texelProperties.address.texelOffset);
        }

        // Unsafe if the previous lock bit is allocated
        // If the coordinate is out of bounds, the texel coordinates will be clamped to its bounds.
        // For concurrency, this will result in inevitable reports. There's some question as to what
        // the valid behaviour is here, today, Reshape will only report errors for in bounds texels,
        // as the bounds feature will snuff out invalid addressing.
        IL::ID unsafeCond = pre.And(pre.NotEqual(previousLock, zero), pre.Not(texelProperties.address.isOutOfBounds));

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(unsafeCond, oobBlock, resumeBlock, IL::ControlFlow::Selection(resumeBlock));

        // Out of bounds block
        IL::Emitter<> unsafe(program, *oobBlock);
        {
            unsafe.AddBlockFlag(BasicBlockFlag::NoInstrumentation);

            // Export the message
            ResourceRaceConditionMessage::ShaderExport msg;
            msg.sguid = unsafe.UInt32(sguid);
            msg.LUID = program.GetConstants().UInt(0)->id;

            // Detailed instrumentation?
            if (config.detail) {
                msg.chunks |= ResourceRaceConditionMessage::Chunk::Detail;
                msg.detail.token = texelProperties.packedToken;
                msg.detail.coordinate[0] = texelProperties.address.x;
                msg.detail.coordinate[1] = texelProperties.address.y;
                msg.detail.coordinate[2] = texelProperties.address.z;
                msg.detail.mip = texelProperties.address.mip;
            }
            
            // Export the message
            unsafe.Export(exportID, msg);

            // Branch back
            unsafe.Branch(resumeBlock);
        }

        // Reads have no lock
        if (!isWrite) {
            return instr;
        }

        // Writes release lock after IOI
        // Clear the lock allocation bit
        IL::Emitter<> resumeEmitter(program, *resumeBlock, ++resumeBlock->begin());
        AtomicClearTexelAddress(resumeEmitter, texelMaskBufferDataID, texelProperties.texelBaseOffsetAlign32, texelProperties.address.texelOffset);

        // Resume after unlock
        return resumeEmitter.GetIterator();
    });
}

void ConcurrencyFeature::OnCreateResource(const ResourceCreateInfo &source) {
    std::lock_guard guard(mutex);

    // Create allocation
    TexelMemoryAllocation memory = texelAllocator->Allocate(source.resource);

    // Create allocation
    Allocation& allocation = allocations[source.resource.token.puid];
    allocation.memory = memory;

    // Mark for pending enqueue
    pendingMappingQueue.push_back(MappingTag {
        .puid = source.resource.token.puid,
        .memoryBaseAlign32 = allocation.memory.texelBaseBlock
    });
}

void ConcurrencyFeature::OnDestroyResource(const ResourceInfo &source) {
    std::lock_guard guard(mutex);

    // Get allocation
    Allocation& allocation = allocations.at(source.token.puid);

    // Free underlying memory
    texelAllocator->Free(allocation.memory);

    // Remove local tracking
    allocations.erase(source.token.puid);
}

void ConcurrencyFeature::OnSubmitBatchBegin(SubmissionContext &submitContext, const CommandContextHandle *contexts, uint32_t contextCount) {
    std::lock_guard guard(mutex);

    // Not interested in empty submissions
    if (!contextCount) {
        return;
    }

    // Any mappings to push?
    if (!pendingMappingQueue.empty())
    {
        // Allocate the next sync value
        ++exclusiveTransferPrimitiveMonotonicCounter;
        
        // Create builder
        CommandBuffer buffer;
        CommandBuilder builder(buffer);

        // Assign the memory lookups
        for (const MappingTag& tag : pendingMappingQueue) {
            // May have been destroyed
            if (!allocations.contains(tag.puid)) {
                continue;
            }

            // Get allocation
            Allocation& allocation = allocations[tag.puid];

            // Assign the PUID -> Memory Offset mapping
            builder.StageBuffer(puidMemoryBaseBufferID, tag.puid * sizeof(uint32_t), sizeof(uint32_t), &tag.memoryBaseAlign32);

            // Initialize texel data
            texelAllocator->Initialize(builder, allocation.memory);
        }

        // Update the residency of all texels
        texelAllocator->UpdateResidency(Queue::ExclusiveTransfer);

        // Clear mappings
        pendingMappingQueue.clear();

        // Submit to the transfer queue
        SchedulerPrimitiveEvent event;
        event.id = exclusiveTransferPrimitiveID;
        event.value = exclusiveTransferPrimitiveMonotonicCounter;
        scheduler->Schedule(Queue::ExclusiveTransfer, buffer, &event);
    }

    // Submissions always wait for the last mappings
    submitContext.waitPrimitives.Add(SchedulerPrimitiveEvent {
        .id = exclusiveTransferPrimitiveID,
        .value = exclusiveTransferPrimitiveMonotonicCounter
    });
}

FeatureInfo ConcurrencyFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Concurrency";
    info.description = "Instrumentation and validation of race conditions across events or queues";

    // Resource bounds requires valid descriptor data, for proper safe-guarding add the descriptor feature as a dependency.
    // This ensures that during instrumentation, we are operating on the already validated, and potentially safe-guarded, descriptor data.
    info.dependencies.push_back(DescriptorFeature::kID);

    // OK
    return info;
}
