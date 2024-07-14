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
#include <Features/Concurrency/ValidationListener.h>
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
#include <Backend/IL/Analysis/StructuralUserAnalysis.h>

// Addressing
#include <Addressing/TexelMemoryAllocator.h>
#include <Addressing/IL/BitIndexing.h>
#include <Addressing/IL/Emitters/TexelPropertiesEmitter.h>

// Generated schema
#include <Schemas/Features/Concurrency.h>
#include <Schemas/Instrumentation.h>
#include <Schemas/InstrumentationCommon.h>

#if CONCURRENCY_ENABLE_VALIDATION
// Bridge
#include <Bridge/IBridge.h>
#endif // CONCURRENCY_ENABLE_VALIDATION

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

#if CONCURRENCY_ENABLE_VALIDATION
    // Create listener
    validationListener = registry->New<ConcurrencyValidationListener>(container);

    // Register to bridge
    registry->Get<IBridge>()->Register(ResourceRaceConditionMessage::kID, validationListener);
#endif // CONCURRENCY_ENABLE_VALIDATION

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

void ConcurrencyFeature::PreInject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Analyze structural usage for all source users
    program.GetAnalysisMap().FindPassOrCompute<IL::StructuralUserAnalysis>(program);
}

void ConcurrencyFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void ConcurrencyFeature::Activate(FeatureActivationStage stage) {
    std::lock_guard guard(container.mutex);
    
    switch (stage) {
        default: {
            break;
        }
        case FeatureActivationStage::Instrumentation: {
            // Slowly start mapping allocations, as many as we can before the actual commit
            incrementalMapping = true;
            break;
        }
        case FeatureActivationStage::Commit: {
            // Pipelines are about to be committed, get all the allocations ready
            // Next submission will pick it up
            MapPendingAllocationsNoLock();

            // Disable incremental
            incrementalMapping = false;
            break;
        }
    }
    
    // Treat as activated
    activated = true;
}

void ConcurrencyFeature::Deactivate() {
    activated = false;
}

void ConcurrencyFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const SetInstrumentationConfigMessage config = CollapseOrDefault<SetInstrumentationConfigMessage>(specialization);
    
    // Get the data ids
    IL::ID puidMemoryBaseBufferDataID = program.GetShaderDataMap().Get(puidMemoryBaseBufferID)->id;
    IL::ID texelMaskBufferDataID      = program.GetShaderDataMap().Get(texelAllocator->GetTexelBlocksBufferID())->id;

    // Common constants
    IL::ID zero = program.GetConstants().UInt(0)->id;
    IL::ID untracked = program.GetConstants().UInt(static_cast<uint32_t>(FailureCode::Untracked))->id;

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Is write operation?
        bool isWrite = false;

        // Instruction of interest?
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::LoadBuffer:
            case IL::OpCode::LoadBufferRaw:
            case IL::OpCode::SampleTexture: {
                break;
            }
            case IL::OpCode::StoreBuffer:
            case IL::OpCode::StoreBufferRaw:
            case IL::OpCode::StoreTexture: {
                isWrite = true;
                break;
            }
            case IL::OpCode::LoadTexture: {
                IL::ID resource = it->As<IL::LoadTextureInstruction>()->texture;

                // Get type
                auto type = program.GetTypeMap().GetType(resource)->As<Backend::IL::TextureType>();

                // Sub-pass inputs are not validated
                if (type->dimension == Backend::IL::TextureDimension::SubPass) {
                    return it;
                }
                break;
            }
            case IL::OpCode::Load: {
                auto _instr = it->As<IL::LoadInstruction>();
    
                // Quick check, if the address space isn't resource related, ignore it
                auto type = program.GetTypeMap().GetType(_instr->address)->As<Backend::IL::PointerType>();
                if (!IsGenericResourceAddressSpace(type)) {
                    return it;
                }

                // Try to find the resource being addressed,
                // if this either fails, or we're just loading the resource itself, ignore it
                IL::ID resourceAddress = Backend::IL::GetResourceFromAddressChain(program, _instr->address);
                if (resourceAddress == IL::InvalidID || resourceAddress == _instr->address) {
                    return it;
                }

                // OK
                break;
            }
            case IL::OpCode::Store: {
                auto _instr = it->As<IL::StoreInstruction>();
                
                // Quick check, if the address space isn't resource related, ignore it
                auto type = program.GetTypeMap().GetType(_instr->address)->As<Backend::IL::PointerType>();
                if (!IsGenericResourceAddressSpace(type)) {
                    return it;
                }

                // Try to find the resource being addressed,
                // if this either fails, or we're just loading the resource itself, ignore it
                IL::ID resourceAddress = Backend::IL::GetResourceFromAddressChain(program, _instr->address);
                if (resourceAddress == IL::InvalidID || resourceAddress == _instr->address) {
                    return it;
                }

                // OK
                isWrite = true;
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

        // If untracked, don't write or read any bits
        IL::ID isUntracked = pre.Equal(texelProperties.failureBlock, untracked);

        // Is this texel range unsafe? i.e., modifications are invalid?
        IL::ID unsafeTexelRange = pre.Or(texelProperties.address.isOutOfBounds, isUntracked);

        // Debug. If the addressing is invalid, the range is definitely unsafe.
#if CONCURRENCY_ENABLE_VALIDATION && TEXEL_ADDRESSING_ENABLE_FENCING
        unsafeTexelRange = pre.Or(unsafeTexelRange, texelProperties.invalidAddressing);
#endif // CONCURRENCY_ENABLE_VALIDATION && TEXEL_ADDRESSING_ENABLE_FENCING

        // Manually select the target bit, this follows the same mechanism as the other overloads,
        // in our case we set the target bit to 0 if the address is out of bounds. Effectively disabling
        // the atomic writes without adding block branching logic.
        IL::ID guardedTexelCount = pre.Select(
            unsafeTexelRange,
            program.GetConstants().UInt(0)->id,
            texelProperties.address.texelCount
        );

        // Read the previous lock, semantics change if write
        IL::ID previousLock;
        if (isWrite) {
            // Writes follow the single producer pattern, so, write the destination bit and check if it was already locked
            // Basically an atomic or with the target bit
            previousLock = AtomicOpTexelAddressRegion<RegionCombinerBitOr>(
                pre,
                AtomicOrTexelAddressValue<IL::Op::Append>,
                texelMaskBufferDataID,
                texelProperties.texelBaseOffsetAlign32, texelProperties.address.texelOffset,
                texelProperties.texelCountLiteral, guardedTexelCount
            );
        } else {
            // Reads follow multiple-consumers, so check if anything has locked the destination bit
            previousLock = AtomicOpTexelAddressRegion<RegionCombinerBitOr>(
                pre,
                ReadTexelAddressValue<IL::Op::Append>,
                texelMaskBufferDataID,
                texelProperties.texelBaseOffsetAlign32, texelProperties.address.texelOffset,
                texelProperties.texelCountLiteral, guardedTexelCount
            );
        }

        // Unsafe if the previous lock bit is allocated
        // If the coordinate is out of bounds, the texel coordinates will be clamped to its bounds.
        // For concurrency, this will result in inevitable reports. There's some question as to what
        // the valid behaviour is here, today, Reshape will only report errors for in bounds texels,
        // as the bounds feature will snuff out invalid addressing.
        IL::ID unsafeCond = pre.And(pre.NotEqual(previousLock, zero), pre.Not(texelProperties.address.isOutOfBounds));
        
#if CONCURRENCY_ENABLE_VALIDATION && TEXEL_ADDRESSING_ENABLE_FENCING
        // Under fencing, report just the invalid addressing cases
        IL::ID unsafeCond = texelProperties.invalidAddressing;
#endif // CONCURRENCY_ENABLE_VALIDATION && TEXEL_ADDRESSING_ENABLE_FENCING

        // If untracked, this can never fail
        unsafeCond = pre.And(unsafeCond, pre.Not(isUntracked));
        
        // If so, branch to failure, otherwise resume
        pre.BranchConditional(unsafeCond, oobBlock, resumeBlock, IL::ControlFlow::Selection(resumeBlock));

        // Out of bounds block
        IL::Emitter<> unsafe(program, *oobBlock);
        {
            unsafe.AddBlockFlag(BasicBlockFlag::NoInstrumentation);

            // Export the message
            ResourceRaceConditionMessage::ShaderExport msg;
            msg.sguid = unsafe.UInt32(sguid);
            msg.LUID = unsafe.UInt32(0);

            // Detailed instrumentation?
            if (config.detail) {
                msg.chunks |= ResourceRaceConditionMessage::Chunk::Detail;
                msg.detail.token = texelProperties.packedToken;
                msg.detail.coordinate[0] = texelProperties.address.x;
                msg.detail.coordinate[1] = texelProperties.address.y;
                msg.detail.coordinate[2] = texelProperties.address.z;
                msg.detail.mip = texelProperties.address.mip;
                msg.detail.byteOffset = texelProperties.offset;

                // Under fencing, repurpose the channels
#if CONCURRENCY_ENABLE_VALIDATION
                msg.detail.coordinate[0] = texelProperties.texelBaseOffsetAlign32;
                msg.detail.coordinate[1] = texelProperties.address.texelOffset;
                msg.detail.coordinate[2] = unsafe.UInt32(texelProperties.texelCountLiteral);
                msg.detail.mip = guardedTexelCount;
#if TEXEL_ADDRESSING_ENABLE_FENCING
                msg.detail.byteOffset = texelProperties.resourceTexelCount;
#endif // TEXEL_ADDRESSING_ENABLE_FENCING
#endif // CONCURRENCY_ENABLE_VALIDATION
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
        AtomicOpTexelAddressRegion<RegionCombinerIgnore>(
            resumeEmitter,
            AtomicClearTexelAddressValue<IL::Op::Append>,
            texelMaskBufferDataID, 
            texelProperties.texelBaseOffsetAlign32, texelProperties.address.texelOffset,
            texelProperties.texelCountLiteral, guardedTexelCount
        );

        // Resume after unlock
        return resumeEmitter.GetIterator();
    });
}

void ConcurrencyFeature::MapAllocationNoLock(ConcurrencyContainer::Allocation &allocation) {
    ASSERT(!allocation.mapped, "Allocation double-mapping");

    // Actual creation parameters for texel addressing
    ResourceInfo filteredInfo = allocation.createInfo.resource;

    // If tiled, reduce all (volume) dimensions to 1
    // since it's not actually being tracked, and can have
    // massive size requirements.
    if (allocation.createInfo.createFlags & ResourceCreateFlag::Tiled) {
        filteredInfo.token.width = 1u;
        filteredInfo.token.height = 1u;

        // Non-volumetric resources keep the subresource layout intact
        if (filteredInfo.isVolumetric) {
            filteredInfo.token.depthOrSliceCount = 1u;
        }
    }

    // Create mapping
    allocation.memory = texelAllocator->Allocate(filteredInfo);

    // Mark for pending enqueue
    pendingMappingQueue.push_back(MappingTag {
        .puid = allocation.createInfo.resource.token.puid,
        .memoryBaseAlign32 = allocation.memory.texelBaseBlock
    });

    // Virtual resources are not tracked (yet)
    if (allocation.createInfo.createFlags & ResourceCreateFlag::Tiled) {
        allocation.failureCode = FailureCode::Untracked;
    }

    // Mapped!
    allocation.mapped = true;
}

void ConcurrencyFeature::MapPendingAllocationsNoLock() {
    // Manually map all unmapped allocations
    for (ConcurrencyContainer::Allocation* allocation : container.pendingMappingQueue) {
        MapAllocationNoLock(*allocation);
    }

    // Cleanup
    container.pendingMappingQueue.Clear();
}

void ConcurrencyFeature::OnCreateResource(const ResourceCreateInfo &source) {
    std::lock_guard guard(container.mutex);

    // Create allocation
    ConcurrencyContainer::Allocation& allocation = container.allocations[source.resource.token.puid];
    allocation.createInfo = source;
    allocation.mapped = false;

    // If activated, create it immediately, otherwise add it to the pending queue
    if (activated) {
        MapAllocationNoLock(allocation);
    } else {
        container.pendingMappingQueue.Add(&allocation);
    }
}

void ConcurrencyFeature::OnDestroyResource(const ResourceInfo &source) {
    std::lock_guard guard(container.mutex);

    // Do not fault on app errors
    auto allocationIt = container.allocations.find(source.token.puid);
    if (allocationIt == container.allocations.end()) {
        return;
    }

    // Get allocation
    ConcurrencyContainer::Allocation& allocation = allocationIt->second;
    
    // Free underlying memory if mapped
    if (allocation.mapped) {
        texelAllocator->Free(allocation.memory);
    } else {
        // Still in mapping queue, remove it
        container.pendingMappingQueue.Remove(&allocation);
    }

    // Remove local tracking
    container.allocations.erase(source.token.puid);
}

void ConcurrencyFeature::OnSubmitBatchBegin(SubmissionContext &submitContext, const CommandContextHandle *contexts, uint32_t contextCount) {
    std::lock_guard guard(container.mutex);

    // Not interested in empty submissions
    if (!contextCount) {
        return;
    }

    // Incremental mapping?
    if (incrementalMapping) {
        static constexpr size_t kIncrementalSubmissionBudget = 100;

        // Number of mappings to handle
        size_t mappingCount = std::min(container.pendingMappingQueue.Size(), kIncrementalSubmissionBudget);

        // End of incremental update
        int64_t end = static_cast<int64_t>(container.pendingMappingQueue.Size() - mappingCount);

        // Map from end of container
        for (int64_t i = container.pendingMappingQueue.Size() - 1; i >= end; i--) {
            ConcurrencyContainer::Allocation *allocation = container.pendingMappingQueue[i];
            MapAllocationNoLock(*allocation);

            // Removing from end of container, just pops
            container.pendingMappingQueue.Remove(allocation);
        }
    }

    // Any mappings to push?
    if (!pendingMappingQueue.empty()) {
        // Allocate the next sync value
        ++exclusiveTransferPrimitiveMonotonicCounter;
        
        // Create builder
        CommandBuffer buffer;
        CommandBuilder builder(buffer);

        // Assign the memory lookups
        for (const MappingTag& tag : pendingMappingQueue) {
            // May have been destroyed
            if (!container.allocations.contains(tag.puid)) {
                continue;
            }

            // Get allocation
            ConcurrencyContainer::Allocation& allocation = container.allocations[tag.puid];

            // Assign the PUID -> Memory Offset mapping
            builder.StageBuffer(puidMemoryBaseBufferID, tag.puid * sizeof(uint32_t), sizeof(uint32_t), &tag.memoryBaseAlign32);

            // Initialize texel data
            texelAllocator->Initialize(builder, allocation.memory, static_cast<uint32_t>(allocation.failureCode));
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
