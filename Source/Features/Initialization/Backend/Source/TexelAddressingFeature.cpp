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
#include <Features/Initialization/TexelAddressingFeature.h>
#include <Features/Initialization/TexelAddressing/MaskBlitShaderProgram.h>
#include <Features/Initialization/TexelAddressing/MaskCopyRangeShaderProgram.h>
#include <Features/Descriptor/Feature.h>
#include <Features/Initialization/TexelAddressing/MaskBlitParameters.h>
#include <Features/Initialization/TexelAddressing/MaskCopyRangeParameters.h>
#include <Features/Initialization/TexelAddressing/KernelShared.h>

// Addressing
#include <Addressing/IL/BitIndexing.h>
#include <Addressing/TexelMemoryAllocator.h>
#include <Addressing/IL/TexelProperties.h>
#include <Addressing/IL/Emitters/TexelPropertiesEmitter.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/Emitters/ResourceTokenEmitter.h>
#include <Backend/IL/ResourceTokenType.h>
#include <Backend/CommandContext.h>
#include <Backend/Resource/ResourceInfo.h>
#include <Backend/Command/AttachmentInfo.h>
#include <Backend/Command/RenderPassInfo.h>
#include <Backend/Command/CommandBuilder.h>
#include <Backend/ShaderProgram/IShaderProgramHost.h>
#include <Backend/Scheduler/IScheduler.h>
#include <Backend/SubmissionContext.h>
#include <Backend/Scheduler/SchedulerTileMapping.h>

// Generated schema
#include <Schemas/Features/Initialization.h>
#include <Schemas/Instrumentation.h>
#include <Schemas/InstrumentationCommon.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Common
#include <Common/Registry.h>

/// Debugging toggle
#define USE_METADATA_CLEAR_CHECKS 1

bool TexelAddressingInitializationFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<UninitializedResourceMessage>();

    // Optional SGUID host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Get scheduler
    scheduler = registry->Get<IScheduler>();

    // Create monotonic primitives
    exclusiveTransferPrimitiveID = scheduler->CreatePrimitive();
    exclusiveComputePrimitiveID = scheduler->CreatePrimitive();
    
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

    // Must have program host
    auto programHost = registry->Get<IShaderProgramHost>();
    if (!programHost) {
        return false;
    }

    // Create programs
    if (!CreateBlitPrograms(programHost) || !CreateCopyPrograms(programHost)) {
        return false;
    }

    // OK
    return true;
}

bool TexelAddressingInitializationFeature::PostInstall() {
    // Create pre-initialized (external) null buffer
    OnCreateResource(ResourceCreateInfo {
        .resource = ResourceInfo::Buffer(ResourceToken {
            .puid = IL::kResourceTokenPUIDReservedNullBuffer,
            .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Buffer)
        }),
        .createFlags = ResourceCreateFlag::OpenedFromExternalHandle
    });

    // Create pre-initialized (external) null texture
    OnCreateResource(ResourceCreateInfo {
        .resource = ResourceInfo::Texture(ResourceToken {
            .puid = IL::kResourceTokenPUIDReservedNullTexture,
            .type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture)
        }, false),
        .createFlags = ResourceCreateFlag::OpenedFromExternalHandle
    });

    // OK
    return true;
}

FeatureHookTable TexelAddressingInitializationFeature::GetHookTable() {
    FeatureHookTable table{};
    table.createResource = BindDelegate(this, TexelAddressingInitializationFeature::OnCreateResource);
    table.destroyResource = BindDelegate(this, TexelAddressingInitializationFeature::OnDestroyResource);
    table.mapResource = BindDelegate(this, TexelAddressingInitializationFeature::OnMapResource);
    table.copyResource = BindDelegate(this, TexelAddressingInitializationFeature::OnCopyResource);
    table.resolveResource = BindDelegate(this, TexelAddressingInitializationFeature::OnResolveResource);
    table.clearResource = BindDelegate(this, TexelAddressingInitializationFeature::OnClearResource);
    table.writeResource = BindDelegate(this, TexelAddressingInitializationFeature::OnWriteResource);
    table.discardResource = BindDelegate(this, TexelAddressingInitializationFeature::OnDiscardResource);
    table.beginRenderPass = BindDelegate(this, TexelAddressingInitializationFeature::OnBeginRenderPass);
    table.preSubmit = BindDelegate(this, TexelAddressingInitializationFeature::OnSubmitBatchBegin);
    table.join = BindDelegate(this, TexelAddressingInitializationFeature::OnJoin);
    return table;
}

void TexelAddressingInitializationFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void TexelAddressingInitializationFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void TexelAddressingInitializationFeature::Activate(FeatureActivationStage stage) {
    std::lock_guard guard(mutex);
    
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

void TexelAddressingInitializationFeature::Deactivate() {
    activated = false;
}

void TexelAddressingInitializationFeature::PreInject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Analyze structural usage for all source users
    program.GetAnalysisMap().FindPassOrCompute<IL::StructuralUserAnalysis>(program);
}

void TexelAddressingInitializationFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const SetInstrumentationConfigMessage config = CollapseOrDefault<SetInstrumentationConfigMessage>(specialization);
    
    // Get the data ids
    IL::ID puidMemoryBaseBufferDataID = program.GetShaderDataMap().Get(puidMemoryBaseBufferID)->id;
    IL::ID texelMaskBufferDataID      = program.GetShaderDataMap().Get(texelAllocator->GetTexelBlocksBufferID())->id;

    // Constants
    IL::ID zero = program.GetConstants().UInt(0)->id;
    IL::ID untracked = program.GetConstants().UInt(static_cast<uint32_t>(FailureCode::Untracked))->id;

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Write operation?
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

        // Write operations just assign the new mask
        if (isWrite) {
            IL::InstructionRef ref(it);

            // Insert prior to IOI
            IL::Emitter<> emitter(program, context.basicBlock, it);

            // Get the texel address
            Backend::IL::TexelPropertiesEmitter propertiesEmitter(emitter, texelAllocator, puidMemoryBaseBufferDataID);
            TexelProperties texelProperties = propertiesEmitter.GetTexelProperties(ref);

            // If untracked, don't write any bits
            IL::ID isUntracked = emitter.Equal(texelProperties.failureBlock, untracked);
            
            // Is this texel range unsafe? i.e., modifications are invalid?
#if TEXEL_ADDRESSING_ENABLE_FENCING
            IL::ID unsafeTexelRange = emitter.Or(texelProperties.invalidAddressing, isUntracked);
#else // TEXEL_ADDRESSING_ENABLE_FENCING
            IL::ID unsafeTexelRange = isUntracked;
#endif // TEXEL_ADDRESSING_ENABLE_FENCING

            // If unsafe, do not modify anything
            texelProperties.address.texelCount = emitter.Select(unsafeTexelRange, zero, texelProperties.address.texelCount);

            // Mark it as initialized
            AtomicOpTexelAddressRegion<RegionCombinerIgnore>(
                emitter,
                AtomicOrTexelAddressValue<IL::Op::Append>,
                texelMaskBufferDataID,
                texelProperties.texelBaseOffsetAlign32, texelProperties.address.texelOffset,
                texelProperties.texelCountLiteral, texelProperties.address.texelCount
            );
            
            // Resume on next
            return emitter.GetIterator();
        }

        // Bind the SGUID
        ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

        // Allocate blocks
        IL::BasicBlock* resumeBlock = context.function.GetBasicBlocks().AllocBlock();
        IL::BasicBlock* mismatchBlock = context.function.GetBasicBlocks().AllocBlock();

        // Split this basic block, move all instructions post and including the instrumented instruction to resume
        // ! iterator invalidated
        auto instr = context.basicBlock.Split(resumeBlock, it);

        // Perform instrumentation check
        IL::Emitter<> pre(program, context.basicBlock);

        // Get the texel address
        Backend::IL::TexelPropertiesEmitter propertiesEmitter(pre, texelAllocator, puidMemoryBaseBufferDataID);
        TexelProperties texelProperties = propertiesEmitter.GetTexelProperties(IL::InstructionRef(instr));

        // If untracked, don't read any bits
        IL::ID isUntracked = pre.Equal(texelProperties.failureBlock, untracked);
        
        // Is this texel range unsafe? i.e., modifications are invalid?
#if TEXEL_ADDRESSING_ENABLE_FENCING
        IL::ID unsafeTexelRange = pre.Or(texelProperties.invalidAddressing, isUntracked);
#else // TEXEL_ADDRESSING_ENABLE_FENCING
        IL::ID unsafeTexelRange = isUntracked;
#endif // TEXEL_ADDRESSING_ENABLE_FENCING

        // If unsafe, do not modify anything
        texelProperties.address.texelCount = pre.Select(unsafeTexelRange, zero, texelProperties.address.texelCount);
        
        // Fetch the bits
        // This doesn't need to be atomic, as the source memory should be visible at this point
        IL::ID anyBitsZero = AtomicOpTexelAddressRegion<RegionCombinerAnyNotEqual>(
            pre,
            ReadTexelAddressValue<IL::Op::Append>,
            texelMaskBufferDataID,
            texelProperties.texelBaseOffsetAlign32, texelProperties.address.texelOffset,
            texelProperties.texelCountLiteral, texelProperties.address.texelCount
        );
        
        // Do not track out of bound reads
        IL::ID cond = pre.And(anyBitsZero, pre.Not(texelProperties.address.isOutOfBounds));

        // If the failure block has any data, this is a bad operation
        cond = pre.Or(cond, pre.NotEqual(texelProperties.failureBlock, zero));

        // If unsafe, this can never fail
        cond = pre.And(cond, pre.Not(unsafeTexelRange));

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(cond, mismatchBlock, resumeBlock, IL::ControlFlow::Selection(resumeBlock));

        // Allocate failure block
        {
            IL::Emitter<> mismatch(program, *mismatchBlock);
            mismatch.AddBlockFlag(BasicBlockFlag::NoInstrumentation);
            
            // Setup message
            UninitializedResourceMessage::ShaderExport msg;
            msg.sguid = mismatch.UInt32(sguid);
            msg.failureCode = texelProperties.failureBlock;

            // Detailed instrumentation?
            if (config.detail) {
                msg.chunks |= UninitializedResourceMessage::Chunk::Detail;
                msg.detail.token = texelProperties.packedToken;
                msg.detail.coordinate[0] = texelProperties.address.x;
                msg.detail.coordinate[1] = texelProperties.address.y;
                msg.detail.coordinate[2] = texelProperties.address.z;
                msg.detail.byteOffset = texelProperties.offset;
                msg.detail.mip = texelProperties.address.mip;
            }
            
            // Export the message
            mismatch.Export(exportID, msg);

            // Branch back
            mismatch.Branch(resumeBlock);
        }
        
        return instr;
    });
}

/// Cast a 64 bit value to 32 bit
static uint32_t Cast32Checked(uint64_t value) {
    ASSERT(value < std::numeric_limits<uint32_t>::max(), "Numeric value out of bounds");
    return static_cast<uint32_t>(value);
}

/// Get the number of workgroups
static uint32_t GetWorkgroupCount(uint64_t value, uint32_t align) {
    return Cast32Checked((value + align - 1) / align);
}

void TexelAddressingInitializationFeature::MapAllocationNoLock(Allocation &allocation, bool immediate) {
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

    // Create allocation
    allocation.memory = texelAllocator->Allocate(filteredInfo);

    // Mark for pending enqueue
    pendingMappingQueue.push_back(MappingTag {
        .puid = allocation.createInfo.resource.token.puid,
        .memoryBaseAlign32 = allocation.memory.texelBaseBlock
    });

    // If this texture was opened from an external handle, we just assume
    // everything has been initialized. Carrying initialization states across
    // completely opaque sources is outside the scope of this feature.
    if (allocation.createInfo.createFlags & ResourceCreateFlag::OpenedFromExternalHandle) {
        if (!puidSRBInitializationSet.contains(allocation.createInfo.resource.token.puid)) {
            pendingInitializationQueue.push_back(InitialiationTag {
                .info = allocation.createInfo.resource,
                .srb = ~0u
            });
        }
    }

    // If the resource requires a clear for stable metadata, mark the failure code as such
    // This only occurs in immediate mode, as post-creation tracking will likely produce no relevant data
#if USE_METADATA_CLEAR_CHECKS
    if (immediate && (allocation.createInfo.createFlags & ResourceCreateFlag::MetadataRequiresHardwareClear)) {
        allocation.failureCode = FailureCode::MetadataRequiresHardwareClear;

        // Mark for pending discard
        pendingDiscardQueue.push_back(DiscardTag {
            .puid = allocation.createInfo.resource.token.puid
        });

        // Ensure any submission waits on the queue
        pendingComputeSynchronization = true;
    }
#endif // USE_METADATA_CLEAR_CHECKS

    // Virtual resources are not tracked (yet)
    if (allocation.createInfo.createFlags & ResourceCreateFlag::Tiled) {
        allocation.failureCode = FailureCode::Untracked;
    }

    // Mapped!
    allocation.mapped = true;

    // Was a whole resource blit requested?
    if (allocation.pendingWholeResourceBlit) {
        allocation.pendingWholeResourceBlit = false;
        ScheduleWholeResourceBlit(allocation);
    }
}

void TexelAddressingInitializationFeature::ScheduleWholeResourceBlit(Allocation &allocation) {
    ResourceInfo wholeRange = allocation.createInfo.resource;
    wholeRange.token.DefaultViewToRange();
    wholeRange.token.viewBaseWidth = 0;

    // Default the descriptors
    if (wholeRange.token.GetType() == Backend::IL::ResourceTokenType::Buffer) {
        wholeRange.bufferDescriptor.offset = 0;
        wholeRange.bufferDescriptor.width = wholeRange.token.width;
    } else {
        // Default base region
        wholeRange.textureDescriptor.region.offsetX = 0;
        wholeRange.textureDescriptor.region.offsetY = 0;
        wholeRange.textureDescriptor.region.offsetZ = 0;

        // Default all sub-resources
        wholeRange.textureDescriptor.region.baseMip = 0;
        wholeRange.textureDescriptor.region.baseSlice = 0;
        wholeRange.textureDescriptor.region.mipCount = wholeRange.token.mipCount;

        // Default full extent
        wholeRange.textureDescriptor.region.width = wholeRange.token.width;
        wholeRange.textureDescriptor.region.height = wholeRange.token.height;
        wholeRange.textureDescriptor.region.depth = wholeRange.token.depthOrSliceCount;
    }
    
    // Mark for host initialization
    pendingInitializationQueue.push_back(InitialiationTag {
        .info = wholeRange,
        .srb = ~0u
    });
}

void TexelAddressingInitializationFeature::MapPendingAllocationsNoLock() {
    // Manually map all unmapped allocations
    for (Allocation* allocation : pendingMappingAllocations) {
        MapAllocationNoLock(*allocation, false);
    }

    // Cleanup
    pendingMappingAllocations.Clear();
}

void TexelAddressingInitializationFeature::OnCreateResource(const ResourceCreateInfo &source) {
    std::lock_guard guard(mutex);

    // Validation
    ASSERT(!allocations.contains(source.resource.token.puid), "Double PUID allocation");
    
    // Create allocation
    Allocation& allocation = allocations[source.resource.token.puid];
    allocation.createInfo = source;
    allocation.mapped = false;

    // If activated, create it immediately, otherwise add it to the pending queue
    if (activated) {
        MapAllocationNoLock(allocation, true);
    } else {
        pendingMappingAllocations.Add(&allocation);
    }
}

void TexelAddressingInitializationFeature::OnDestroyResource(const ResourceInfo &source) {
    std::lock_guard guard(mutex);

    // Do not fault on app errors
    auto allocationIt = allocations.find(source.token.puid);
    if (allocationIt == allocations.end()) {
        return;
    }

    // Get allocation
    Allocation& allocation = allocationIt->second;

    // Free underlying memory
    if (allocation.mapped) {
        texelAllocator->Free(allocation.memory);
    } else {
        // Still in mapping queue, remove it
        pendingMappingAllocations.Remove(&allocation);
    }

    // Remove local tracking
    allocations.erase(source.token.puid);
    puidSRBInitializationSet.erase(source.token.puid);
}

void TexelAddressingInitializationFeature::OnMapResource(const ResourceInfo &source) {
    std::lock_guard guard(mutex);

    // Mapping any resource is currently treated as marking the entire thing as initialized
    // This could be tracked on a range basis, but even that doesn't really reflect reality, the
    // way forward really is page guards on the fetched memory, and, also, tracking things on a
    // per byte level.
    
    // Skip if already initialized
    if (puidSRBInitializationSet.contains(source.token.puid)) {
        return;
    }

    // Get allocation
    Allocation& allocation = allocations[source.token.puid];

    // If the allocation has already been mapped to memory, schedule it immediately
    if (allocation.mapped) {
        ScheduleWholeResourceBlit(allocation);
    } else {
        // Not yet mapped, so, let the future map pick up the request
        allocation.pendingWholeResourceBlit = true;
    }
}

void TexelAddressingInitializationFeature::OnCopyResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest) {
    std::lock_guard guard(mutex);
    CopyResourceMaskRange(context->buffer, source, dest);
    OnMetadataInitializationEvent(context, dest);
}

void TexelAddressingInitializationFeature::OnResolveResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest) {
    std::lock_guard guard(mutex);
    // todo[init]: How can we handle resolve mapping sensibly?
    BlitResourceMask(context->buffer, dest);
    OnMetadataInitializationEvent(context, dest);
}

void TexelAddressingInitializationFeature::OnClearResource(CommandContext* context, const ResourceInfo& resource) {
    std::lock_guard guard(mutex);
    BlitResourceMask(context->buffer, resource);
    OnMetadataInitializationEvent(context, resource);
}

void TexelAddressingInitializationFeature::OnWriteResource(CommandContext* context, const ResourceInfo& resource) {
    std::lock_guard guard(mutex);
    BlitResourceMask(context->buffer, resource);
}

void TexelAddressingInitializationFeature::OnDiscardResource(CommandContext *context, const ResourceInfo &resource) {
    std::lock_guard guard(mutex);
    OnMetadataInitializationEvent(context, resource);
}

void TexelAddressingInitializationFeature::OnBeginRenderPass(CommandContext *context, const RenderPassInfo &passInfo) {
    std::lock_guard guard(mutex);
    
    // TODO: Only blit the "active" render pass region
    
    // Initialize all color targets
    for (uint32_t i = 0; i < passInfo.attachmentCount; i++) {
        const AttachmentInfo& info = passInfo.attachments[i];

        // Mark metadata as cleared if needed
        if (info.loadAction == AttachmentAction::Clear) {
            OnMetadataInitializationEvent(context, info.resource);
        }

        // Only mark as initialized if the destination is written to
        if (info.loadAction == AttachmentAction::Clear ||
            info.storeAction == AttachmentAction::Store ||
            info.storeAction == AttachmentAction::Resolve) {
            BlitResourceMask(context->buffer, info.resource);
        }

        // Always mark resolve targets as initialized
        if (info.resolveResource) {
            BlitResourceMask(context->buffer, *info.resolveResource);
            OnMetadataInitializationEvent(context, *info.resolveResource);
        }
    }

    // Has depth?
    if (passInfo.depthAttachment) {
        // Mark metadata as cleared if needed
        if (passInfo.depthAttachment->loadAction == AttachmentAction::Clear) {
            OnMetadataInitializationEvent(context, passInfo.depthAttachment->resource);
        }
        
        // Only mark as initialized if the destination is written to
        if (passInfo.depthAttachment->loadAction == AttachmentAction::Clear ||
            passInfo.depthAttachment->storeAction == AttachmentAction::Store ||
            passInfo.depthAttachment->storeAction == AttachmentAction::Resolve) {
            BlitResourceMask(context->buffer, passInfo.depthAttachment->resource);
        }   

        // Always mark resolve targets as initialized
        if (passInfo.depthAttachment->resolveResource) {
            BlitResourceMask(context->buffer, *passInfo.depthAttachment->resolveResource);
            OnMetadataInitializationEvent(context, *passInfo.depthAttachment->resolveResource);
        }
    }
}

void TexelAddressingInitializationFeature::OnSubmitBatchBegin(SubmissionContext& submitContext, const CommandContextHandle *contexts, uint32_t contextCount) {
    std::lock_guard guard(mutex);

    // Not interested in empty submissions
    if (!contextCount) {
        return;
    }

    // Incremental mapping?
    if (incrementalMapping) {
        static constexpr size_t kIncrementalSubmissionBudget = 100;

        // Number of mappings to handle
        size_t mappingCount = std::min(pendingMappingAllocations.Size(), kIncrementalSubmissionBudget);

        // End of incremental update
        int64_t end = static_cast<int64_t>(pendingMappingAllocations.Size() - mappingCount);

        // Map from end of container
        for (int64_t i = pendingMappingAllocations.Size() - 1; i >= end; i--) {
            Allocation *allocation = pendingMappingAllocations[i];
            MapAllocationNoLock(*allocation, false);

            // Removing from end of container, just pops
            pendingMappingAllocations.Remove(allocation);
        }
    }

    // Force map all allocations pending initialization
    for (const InitialiationTag& tag : pendingInitializationQueue) {
        if (auto it = allocations.find(tag.info.token.puid); it != allocations.end() && !it->second.mapped) {
            ASSERT(it->second.mapped, "Pending initialization without mapping");

            // Removing from pending queue
            pendingMappingAllocations.Remove(&it->second);
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
            if (!allocations.contains(tag.puid)) {
                continue;
            }

            // Get allocation
            Allocation& allocation = allocations[tag.puid];

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

    // Any discards to push?
    if (!pendingDiscardQueue.empty()) {
        // Allocate the next sync value
        ++exclusiveComputePrimitiveMonotonicCounter;
        
        // Create builder
        CommandBuffer buffer;
        CommandBuilder builder(buffer);

        // Discard all resources
        for (const DiscardTag& tag : pendingDiscardQueue) {
            // May have been destroyed
            if (!allocations.contains(tag.puid)) {
                continue;
            }

            // Discard the entire resource
            builder.Discard(tag.puid);
        }

        // Clear discards
        pendingDiscardQueue.clear();

        // Submit to the compute queue
        SchedulerPrimitiveEvent event;
        event.id = exclusiveComputePrimitiveID;
        event.value = exclusiveComputePrimitiveMonotonicCounter;
        scheduler->Schedule(Queue::Compute, buffer, &event);
    }

    // Submissions always wait for the last mappings
    submitContext.waitPrimitives.Add(SchedulerPrimitiveEvent {
        .id = exclusiveTransferPrimitiveID,
        .value = exclusiveTransferPrimitiveMonotonicCounter
    });

    // If needed, wait on the last discards
    if (pendingComputeSynchronization) {
        submitContext.waitPrimitives.Add(SchedulerPrimitiveEvent {
            .id = exclusiveComputePrimitiveID,
            .value = exclusiveComputePrimitiveMonotonicCounter
        });
    }

    // Set commit base
    // Note: We track on the first context, once the first context has completed, all the rest have
    CommandContextInfo& info = commandContexts[contexts[0]];
    info.committedInitializationHead = committedInitializationBase + pendingInitializationQueue.size();
    
    // Initialize all PUIDs
    for (const InitialiationTag& tag : pendingInitializationQueue) {
        auto it = allocations.find(tag.info.token.puid);
        
        // May have been destroyed
        if (it == allocations.end()) {
            continue;
        }

        // Mark device side initialization
        BlitResourceMask(submitContext.preContext->buffer, tag.info);
    }
}

void TexelAddressingInitializationFeature::OnJoin(CommandContextHandle contextHandle) {
    std::lock_guard guard(mutex);

    // If untracked, ignore
    auto it = commandContexts.find(contextHandle);
    if (it == commandContexts.end()) {
        return;
    }
    
    // If the head is less than the current base, it's already been shaved off
    if (it->second.committedInitializationHead > committedInitializationBase) {
        const size_t shaveCount = it->second.committedInitializationHead - committedInitializationBase;

        // Mark all resources in range as initialized
        for (size_t i = 0; i < shaveCount; i++) {
            puidSRBInitializationSet.insert(pendingInitializationQueue[i].info.token.puid);
        }
        
        // Shave off all known committed initialization events
        pendingInitializationQueue.erase(pendingInitializationQueue.begin(), pendingInitializationQueue.begin() + shaveCount);

        // Set new base
        committedInitializationBase = it->second.committedInitializationHead;
    }

    // Get rid of context
    commandContexts.erase(contextHandle);
}

void TexelAddressingInitializationFeature::OnMetadataInitializationEvent(CommandContext *context, const ResourceInfo &info) {
    // If this is a resource with metadata clear requirements, mark the block as safe now
    if (Allocation& allocation = allocations[info.token.puid]; allocation.failureCode == FailureCode::MetadataRequiresHardwareClear) {
        CommandBuilder builder(context->buffer);
        texelAllocator->StageFailureCode(builder, allocation.memory, 0x0);
    }
}

/// Execute a program in a chunked fashion, respecting the API limits
/// \param builder destination builder
/// \param program program to execute, must be bound prior
/// \param params parameters updated every iteration
/// \param width absolute width
template<typename T, typename P>
static void ChunkedExecution(CommandBuilder& builder, const ComRef<T>& program, P& params, uint64_t width) {
    // Determine the total number of chunks
    uint64_t chunkCount = GetWorkgroupCount(width, kKernelSize.threadsX);

    // Execute all chunks
    for (uint64_t offset = 0; offset < chunkCount; offset += kMaxDispatchThreadGroupPerDimension) {
        auto workgroupCount = std::min<uint32_t>(kMaxDispatchThreadGroupPerDimension, Cast32Checked(chunkCount - offset));

        // Number of threads for this chunk group
        uint32_t threadCount = workgroupCount * kKernelSize.threadsX;

        // Total width of this chunk group
        params.dispatchWidth = std::min(threadCount, static_cast<uint32_t>(width - params.dispatchOffset));

        // Dispatch the current chunk size
        builder.SetDescriptorData(program->GetDataID(), params);
        builder.Dispatch(workgroupCount, 1, 1);

        // Advance offset
        params.dispatchOffset += threadCount;
    }

    // Finalize all writes before potential usage
    builder.UAVBarrier();
}

void TexelAddressingInitializationFeature::BlitResourceMask(CommandBuffer& buffer, const ResourceInfo &info) {
    const ResourceProgram<MaskBlitShaderProgram>& program = blitPrograms[{info.token.GetType(), info.isVolumetric}];
    
    // todo[init]: cpu side copy check
    const Allocation& allocation = allocations.at(info.token.puid);

    // Setup blit parameters
    MaskBlitParameters params{};
    params.memoryBaseElementAlign32 = allocation.memory.texelBaseBlock;

    // Buffers are linearly indexed
    if (info.token.GetType() == Backend::IL::ResourceTokenType::Buffer) {
        ASSERT(!info.bufferDescriptor.placedDescriptor, "Blitting with placement resource");
        ASSERT(info.bufferDescriptor.width <= allocation.memory.addressInfo.texelCount, "Length of of bounds");
        ASSERT(info.bufferDescriptor.offset + info.bufferDescriptor.width <= allocation.memory.addressInfo.texelCount, "End offset of of bounds");

        // Offset by descriptor
        params.baseX = Cast32Checked(info.bufferDescriptor.offset);
        
        // Setup the command
        CommandBuilder builder(buffer);
        builder.SetShaderProgram(program.id);
        builder.SetDescriptorData(program.program->GetDestTokenID(), info.token);
        ChunkedExecution(builder, program.program, params, info.bufferDescriptor.width);
    } else {
        // Blit each mip separately
        for (uint32_t i = 0; i < info.textureDescriptor.region.mipCount; i++) {
            params.mip = info.textureDescriptor.region.baseMip + i;

            // Base offsets
            params.baseX = Cast32Checked(info.textureDescriptor.region.offsetX) >> i;
            params.baseY = Cast32Checked(info.textureDescriptor.region.offsetY) >> i;
            params.baseZ = Cast32Checked(info.textureDescriptor.region.offsetZ) >> i;

            // If volumetric, just append the base slice
            if (!info.isVolumetric) {
                params.baseZ += info.textureDescriptor.region.baseSlice;
            }

            // Expected dimensions
            params.width = info.textureDescriptor.region.width >> i;
            params.height = info.textureDescriptor.region.height >> i;
            // todo[init]: not volumetric
            params.depth = info.textureDescriptor.region.depth >> i;
        
            // Blit the given range int he mip
            CommandBuilder builder(buffer);
            builder.SetShaderProgram(program.id);
            builder.SetDescriptorData(program.program->GetDataID(), params);
            builder.SetDescriptorData(program.program->GetDestTokenID(), info.token);
            ChunkedExecution(builder, program.program, params, params.width * params.height * params.depth);
        }
    }
}

void TexelAddressingInitializationFeature::CopyResourceMaskRange(CommandBuffer& buffer, const ResourceInfo &source, const ResourceInfo &dest) {
    if (source.token.type == dest.token.type) {
        CopyResourceMaskRangeSymmetric(buffer, source, dest);
    } else {
        CopyResourceMaskRangeAsymmetric(buffer, source, dest);
    }
}

void TexelAddressingInitializationFeature::CopyResourceMaskRangeSymmetric(CommandBuffer &buffer, const ResourceInfo &source, const ResourceInfo &dest) {
    const ResourceProgram<MaskCopyRangeShaderProgram>& program = copyPrograms[{source.token.GetType(), dest.token.GetType(), source.isVolumetric}];
    
    // todo[init]: cpu side copy check
    const Allocation& sourceAllocation = allocations.at(source.token.puid);
    const Allocation& destAllocation = allocations.at(dest.token.puid);

    // Setup blit parameters
    MaskCopyRangeParameters params{};
    params.sourceMemoryBaseElementAlign32 = sourceAllocation.memory.texelBaseBlock;
    params.destMemoryBaseElementAlign32 = destAllocation.memory.texelBaseBlock;
    
    // Buffers are linearly indexed
    if (source.token.GetType() == Backend::IL::ResourceTokenType::Buffer) {
        params.sourceBaseX = Cast32Checked(source.bufferDescriptor.offset);
        params.destBaseX = Cast32Checked(dest.bufferDescriptor.offset);
        
        // Setup the command
        CommandBuilder builder(buffer);
        builder.SetShaderProgram(program.id);
        builder.SetDescriptorData(program.program->GetSourceTokenID(), source.token);
        builder.SetDescriptorData(program.program->GetDestTokenID(), dest.token);
        ChunkedExecution(builder, program.program, params, source.bufferDescriptor.width);
    } else {
        // Copy each mip separately
        for (uint32_t i = 0; i < source.textureDescriptor.region.mipCount; i++) {
            params.sourceMip = source.textureDescriptor.region.baseMip + i;
            params.destMip = dest.textureDescriptor.region.baseMip + i;

            // Source base offsets
            params.sourceBaseX = Cast32Checked(source.textureDescriptor.region.offsetX) >> i;
            params.sourceBaseY = Cast32Checked(source.textureDescriptor.region.offsetY) >> i;
            params.sourceBaseZ = Cast32Checked(source.textureDescriptor.region.offsetZ) >> i;

            // Destination base offsets
            params.destBaseX = Cast32Checked(dest.textureDescriptor.region.offsetX) >> i;
            params.destBaseY = Cast32Checked(dest.textureDescriptor.region.offsetY) >> i;
            params.destBaseZ = Cast32Checked(dest.textureDescriptor.region.offsetZ) >> i;

            // If not volumetric, just append the slices
            if (!source.isVolumetric) {
                params.sourceBaseZ += source.textureDescriptor.region.baseSlice;
            }
            
            if (!dest.isVolumetric) {
                params.destBaseZ += dest.textureDescriptor.region.baseSlice;
            }

            // Expected dimensions
            params.width = source.textureDescriptor.region.width >> i;
            params.height = source.textureDescriptor.region.height >> i;
            params.depth = source.textureDescriptor.region.depth >> i;
        
            // Blit the entire range
            CommandBuilder builder(buffer);
            builder.SetShaderProgram(program.id);
            builder.SetDescriptorData(program.program->GetDataID(), params);
            builder.SetDescriptorData(program.program->GetSourceTokenID(), source.token);
            builder.SetDescriptorData(program.program->GetDestTokenID(), dest.token);
            ChunkedExecution(builder, program.program, params, params.width * params.height * params.depth);
        }
    }
}

void TexelAddressingInitializationFeature::CopyResourceMaskRangeAsymmetric(CommandBuffer &buffer, const ResourceInfo &source, const ResourceInfo &dest) {
    // This is an asymmetric copy, if either is volumetric, the copy is
    bool isVolumetric = source.isVolumetric || dest.isVolumetric;

    // Get program
    const ResourceProgram<MaskCopyRangeShaderProgram>& program = copyPrograms[{source.token.GetType(), dest.token.GetType(), isVolumetric}];
    
    // todo[init]: cpu side copy check
    const Allocation& sourceAllocation = allocations.at(source.token.puid);
    const Allocation& destAllocation = allocations.at(dest.token.puid);

    // Setup blit parameters
    MaskCopyRangeParameters params{};
    params.sourceMemoryBaseElementAlign32 = sourceAllocation.memory.texelBaseBlock;
    params.destMemoryBaseElementAlign32 = destAllocation.memory.texelBaseBlock;
    
    // Buffer -> Texture
    if (source.token.GetType() == Backend::IL::ResourceTokenType::Buffer) {
        ASSERT(source.bufferDescriptor.width <= sourceAllocation.memory.addressInfo.texelCount, "Length of of bounds");
        ASSERT(source.bufferDescriptor.offset + source.bufferDescriptor.width <= sourceAllocation.memory.addressInfo.texelCount, "End offset of of bounds");

        // Source buffer offsetting
        params.sourceBaseX = Cast32Checked(source.bufferDescriptor.offset);

        // Placement data
        params.placementRowLength = source.bufferDescriptor.placedDescriptor->rowLength;
        params.placementImageHeight = source.bufferDescriptor.placedDescriptor->imageHeight;
        
        // Destination base offsets
        params.destMip = dest.textureDescriptor.region.baseMip;
        params.destBaseX = dest.textureDescriptor.region.offsetX;
        params.destBaseY = dest.textureDescriptor.region.offsetY;
        params.destBaseZ = dest.textureDescriptor.region.offsetZ;

        // If not volumetric, just append the slices
        if (!dest.isVolumetric) {
            params.destBaseZ += dest.textureDescriptor.region.baseSlice;
        }

        // Set dimensions
        params.width = dest.textureDescriptor.region.width;
        params.height = dest.textureDescriptor.region.height;
        params.depth = dest.textureDescriptor.region.depth;

        // Blit the given range
        CommandBuilder builder(buffer);
        builder.SetShaderProgram(program.id);
        builder.SetDescriptorData(program.program->GetDataID(), params);
        builder.SetDescriptorData(program.program->GetSourceTokenID(), source.token);
        builder.SetDescriptorData(program.program->GetDestTokenID(), dest.token);
        ChunkedExecution(builder, program.program, params, params.width * params.height * params.depth);
    } else {
        // Texture -> Buffer

        // Destination buffer offsetting
        params.destBaseX = Cast32Checked(dest.bufferDescriptor.offset);

        // Placement data
        params.placementRowLength = dest.bufferDescriptor.placedDescriptor->rowLength;
        params.placementImageHeight = dest.bufferDescriptor.placedDescriptor->imageHeight;
        
        // Source base offsets
        params.sourceMip = source.textureDescriptor.region.baseMip;
        params.sourceBaseX = source.textureDescriptor.region.offsetX;
        params.sourceBaseY = source.textureDescriptor.region.offsetY;
        params.sourceBaseZ = source.textureDescriptor.region.offsetZ;

        // If not volumetric, just append the slices
        if (!source.isVolumetric) {
            params.sourceBaseZ += source.textureDescriptor.region.baseSlice;
        }

        // Set dimensions
        params.width = source.textureDescriptor.region.width;
        params.height = source.textureDescriptor.region.height;
        params.depth = source.textureDescriptor.region.depth;

        // Blit the given range
        CommandBuilder builder(buffer);
        builder.SetShaderProgram(program.id);
        builder.SetDescriptorData(program.program->GetDataID(), params);
        builder.SetDescriptorData(program.program->GetSourceTokenID(), source.token);
        builder.SetDescriptorData(program.program->GetDestTokenID(), dest.token);
        ChunkedExecution(builder, program.program, params, params.width * params.height * params.depth);
    }
}

template<typename T, typename ... A>
bool TexelAddressingInitializationFeature::CreateProgram(const ComRef<IShaderProgramHost> &programHost, ResourceProgram<T> &out, A &&...args) {
    out.program = registry->New<T>(args...);

    // Try to install program
    if (!out.program->Install()) {
        return false;
    }

    // Register maskers
    out.id = programHost->Register(out.program);
    return true;
}

bool TexelAddressingInitializationFeature::CreateBlitProgram(const ComRef<IShaderProgramHost> &programHost, Backend::IL::ResourceTokenType type, bool isVolumetric) {
    return CreateProgram(programHost, blitPrograms[{type, isVolumetric}], texelAllocator->GetTexelBlocksBufferID(), type, isVolumetric);
}

bool TexelAddressingInitializationFeature::CreateCopyProgram(const ComRef<IShaderProgramHost> &programHost, Backend::IL::ResourceTokenType from, Backend::IL::ResourceTokenType to, bool isVolumetric) {
    return CreateProgram(programHost, copyPrograms[{from, to, isVolumetric}], texelAllocator->GetTexelBlocksBufferID(), from, to, isVolumetric);
}

bool TexelAddressingInitializationFeature::CreateBlitPrograms(const ComRef<IShaderProgramHost> &programHost) {
    return CreateBlitProgram(programHost, Backend::IL::ResourceTokenType::Buffer, false) &&
           CreateBlitProgram(programHost, Backend::IL::ResourceTokenType::Texture, false) &&
           CreateBlitProgram(programHost, Backend::IL::ResourceTokenType::Texture, true);
}

bool TexelAddressingInitializationFeature::CreateCopyPrograms(const ComRef<IShaderProgramHost> &programHost) {
    // Matching types
    if (!CreateCopyProgram(programHost, Backend::IL::ResourceTokenType::Buffer, Backend::IL::ResourceTokenType::Buffer, false) ||
        !CreateCopyProgram(programHost, Backend::IL::ResourceTokenType::Texture, Backend::IL::ResourceTokenType::Texture, false) ||
        !CreateCopyProgram(programHost, Backend::IL::ResourceTokenType::Texture, Backend::IL::ResourceTokenType::Texture, true)) {
        return false;
    }

    // Placement copies
    if (!CreateCopyProgram(programHost, Backend::IL::ResourceTokenType::Buffer, Backend::IL::ResourceTokenType::Texture, false) ||
        !CreateCopyProgram(programHost, Backend::IL::ResourceTokenType::Buffer, Backend::IL::ResourceTokenType::Texture, true) ||
        !CreateCopyProgram(programHost, Backend::IL::ResourceTokenType::Texture, Backend::IL::ResourceTokenType::Buffer, false) ||
        !CreateCopyProgram(programHost, Backend::IL::ResourceTokenType::Texture, Backend::IL::ResourceTokenType::Buffer, true)) {
        return false;
    }

    // OK
    return true;
}

FeatureInfo TexelAddressingInitializationFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Initialization";
    info.description = "Instrumentation and validation of resource initialization prior to reads";

    // Resource bounds requires valid descriptor data, for proper safe-guarding add the descriptor feature as a dependency.
    // This ensures that during instrumentation, we are operating on the already validated, and potentially safe-guarded, descriptor data.
    info.dependencies.push_back(DescriptorFeature::kID);

    // OK
    return info;
}
