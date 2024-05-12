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
#include <Features/Initialization/Feature.h>
#include <Features/Initialization/MaskBlitShaderProgram.h>
#include <Features/Initialization/MaskCopyRangeShaderProgram.h>
#include <Features/Descriptor/Feature.h>
#include <Features/Initialization/MaskBlitParameters.h>
#include <Features/Initialization/MaskCopyRangeParameters.h>
#include <Features/Initialization/KernelShared.h>

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

bool InitializationFeature::Install() {
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

bool InitializationFeature::PostInstall() {
    // Command buffer for stages
    CommandBuffer  buffer;
    CommandBuilder builder(buffer);

    // Default sub resource base mask
    uint32_t fullSRB = ~0u;

    // Mark null resources as initialized
    // todo[init]: null!
#if 0
    builder.StageBuffer(initializationMaskBufferID, IL::kResourceTokenPUIDReservedNullBuffer * sizeof(uint32_t), sizeof(uint32_t), &fullSRB);
    builder.StageBuffer(initializationMaskBufferID, IL::kResourceTokenPUIDReservedNullTexture * sizeof(uint32_t), sizeof(uint32_t), &fullSRB);
#endif // 0
    
    // Schedule blocking
    scheduler->Schedule(Queue::Graphics, buffer, nullptr);
    scheduler->WaitForPending();

    // OK
    return true;
}

FeatureHookTable InitializationFeature::GetHookTable() {
    FeatureHookTable table{};
    table.createResource = BindDelegate(this, InitializationFeature::OnCreateResource);
    table.destroyResource = BindDelegate(this, InitializationFeature::OnDestroyResource);
    table.mapResource = BindDelegate(this, InitializationFeature::OnMapResource);
    table.copyResource = BindDelegate(this, InitializationFeature::OnCopyResource);
    table.resolveResource = BindDelegate(this, InitializationFeature::OnResolveResource);
    table.clearResource = BindDelegate(this, InitializationFeature::OnClearResource);
    table.writeResource = BindDelegate(this, InitializationFeature::OnWriteResource);
    table.discardResource = BindDelegate(this, InitializationFeature::OnDiscardResource);
    table.beginRenderPass = BindDelegate(this, InitializationFeature::OnBeginRenderPass);
    table.preSubmit = BindDelegate(this, InitializationFeature::OnSubmitBatchBegin);
    table.join = BindDelegate(this, InitializationFeature::OnJoin);
    return table;
}

void InitializationFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void InitializationFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void InitializationFeature::PreInject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Analyze structural usage for all source users
    program.GetAnalysisMap().FindPassOrCompute<IL::StructuralUserAnalysis>(program);
}

void InitializationFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const SetInstrumentationConfigMessage config = CollapseOrDefault<SetInstrumentationConfigMessage>(specialization);
    
    // Get the data ids
    IL::ID puidMemoryBaseBufferDataID = program.GetShaderDataMap().Get(puidMemoryBaseBufferID)->id;
    IL::ID texelMaskBufferDataID      = program.GetShaderDataMap().Get(texelAllocator->GetTexelBlocksBufferID())->id;

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
            case IL::OpCode::StoreTexture:
            case IL::OpCode::StoreBufferRaw: {
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

        // Constants
        IL::ID zero = program.GetConstants().UInt(0)->id;

        // Bind the SGUID
        ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

        // Allocate resume
        IL::BasicBlock* resumeBlock = context.function.GetBasicBlocks().AllocBlock();

        // Split this basic block, move all instructions post and including the instrumented instruction to resume
        // ! iterator invalidated
        auto instr = context.basicBlock.Split(resumeBlock, it);

        // Allocate failure block
        IL::Emitter<> mismatch(program, *context.function.GetBasicBlocks().AllocBlock());
        mismatch.AddBlockFlag(BasicBlockFlag::NoInstrumentation);

        // Perform instrumentation check
        IL::Emitter<> pre(program, context.basicBlock);

        // Get the texel address
        Backend::IL::TexelPropertiesEmitter propertiesEmitter(pre, texelAllocator, puidMemoryBaseBufferDataID);
        TexelProperties texelProperties = propertiesEmitter.GetTexelProperties(IL::InstructionRef(instr));

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

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(cond, mismatch.GetBasicBlock(), resumeBlock, IL::ControlFlow::Selection(resumeBlock));

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

void InitializationFeature::OnCreateResource(const ResourceCreateInfo &source) {
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

    // If this texture was opened from an external handle, we just assume
    // everything has been initialized. Carrying initialization states across
    // completely opaque sources is outside the scope of this feature.
    if (source.createFlags & ResourceCreateFlag::OpenedFromExternalHandle) {
        if (!puidSRBInitializationSet.contains(source.resource.token.puid)) {
            pendingInitializationQueue.push_back(InitialiationTag {
                .info = source.resource,
                .srb = ~0u
            });
        }
    }

    // If the resource requires a clear for stable metadata, mark the failure code as such
#if USE_METADATA_CLEAR_CHECKS
    if (source.createFlags & ResourceCreateFlag::MetadataRequiresHardwareClear) {
        allocation.failureCode = FailureCode::MetadataRequiresHardwareClear;
    }
#endif // USE_METADATA_CLEAR_CHECKS
}

void InitializationFeature::OnDestroyResource(const ResourceInfo &source) {
    std::lock_guard guard(mutex);

    // Get allocation
    Allocation& allocation = allocations.at(source.token.puid);

    // Free underlying memory
    texelAllocator->Free(allocation.memory);

    // Remove local tracking
    allocations.erase(source.token.puid);
    puidSRBInitializationSet.erase(source.token.puid);
}

void InitializationFeature::OnMapResource(const ResourceInfo &source) {
    std::lock_guard guard(mutex);

    // Skip if already initialized
    if (puidSRBInitializationSet.contains(source.token.puid)) {
        return;
    }

    // Mapping any resource is currently treated as marking the entire thing as initialized
    // This could be tracked on a range basis, but even that doesn't really reflect reality, the
    // way forward really is page guards on the fetched memory, and, also, tracking things on a
    // per byte level.
    ResourceInfo wholeRange = source;
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

void InitializationFeature::OnCopyResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest) {
    std::lock_guard guard(mutex);
    CopyResourceMaskRange(context->buffer, source, dest);
    OnMetadataInitializationEvent(context, dest);
}

void InitializationFeature::OnResolveResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest) {
    std::lock_guard guard(mutex);
    // todo[init]: How can we handle resolve mapping sensibly?
    BlitResourceMask(context->buffer, dest);
    OnMetadataInitializationEvent(context, dest);
}

void InitializationFeature::OnClearResource(CommandContext* context, const ResourceInfo& resource) {
    std::lock_guard guard(mutex);
    BlitResourceMask(context->buffer, resource);
    OnMetadataInitializationEvent(context, resource);
}

void InitializationFeature::OnWriteResource(CommandContext* context, const ResourceInfo& resource) {
    std::lock_guard guard(mutex);
    BlitResourceMask(context->buffer, resource);
}

void InitializationFeature::OnDiscardResource(CommandContext *context, const ResourceInfo &resource) {
    std::lock_guard guard(mutex);
    OnMetadataInitializationEvent(context, resource);
}

void InitializationFeature::OnBeginRenderPass(CommandContext *context, const RenderPassInfo &passInfo) {
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

void InitializationFeature::OnSubmitBatchBegin(SubmissionContext& submitContext, const CommandContextHandle *contexts, uint32_t contextCount) {
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

    // Set commit base
    // Note: We track on the first context, once the first context has completed, all the rest have
    CommandContextInfo& info = commandContexts[contexts[0]];
    info.committedInitializationHead = committedInitializationBase + pendingInitializationQueue.size();
    
    // Initialize all PUIDs
    for (const InitialiationTag& tag : pendingInitializationQueue) {
        // May have been destroyed
        if (!allocations.contains(tag.info.token.puid)) {
            continue;
        }

        // Mark device side initialization
        BlitResourceMask(submitContext.preContext->buffer, tag.info);
    }
}

void InitializationFeature::OnJoin(CommandContextHandle contextHandle) {
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

void InitializationFeature::OnMetadataInitializationEvent(CommandContext *context, const ResourceInfo &info) {
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

        // Dispatch the current chunk size
        builder.SetDescriptorData(program->GetDataID(), params);
        builder.Dispatch(workgroupCount, 1, 1);

        // Advance offset
        params.dispatchOffset += workgroupCount * kKernelSize.threadsX;
    }

    // Finalize all writes before potential usage
    builder.UAVBarrier();
}

void InitializationFeature::BlitResourceMask(CommandBuffer& buffer, const ResourceInfo &info) {
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

void InitializationFeature::CopyResourceMaskRange(CommandBuffer& buffer, const ResourceInfo &source, const ResourceInfo &dest) {
    if (source.token.type == dest.token.type) {
        CopyResourceMaskRangeSymmetric(buffer, source, dest);
    } else {
        CopyResourceMaskRangeAsymmetric(buffer, source, dest);
    }
}

void InitializationFeature::CopyResourceMaskRangeSymmetric(CommandBuffer &buffer, const ResourceInfo &source, const ResourceInfo &dest) {
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

void InitializationFeature::CopyResourceMaskRangeAsymmetric(CommandBuffer &buffer, const ResourceInfo &source, const ResourceInfo &dest) {
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
bool InitializationFeature::CreateProgram(const ComRef<IShaderProgramHost> &programHost, ResourceProgram<T> &out, A &&...args) {
    out.program = registry->New<T>(args...);

    // Try to install program
    if (!out.program->Install()) {
        return false;
    }

    // Register maskers
    out.id = programHost->Register(out.program);
    return true;
}

bool InitializationFeature::CreateBlitProgram(const ComRef<IShaderProgramHost> &programHost, Backend::IL::ResourceTokenType type, bool isVolumetric) {
    return CreateProgram(programHost, blitPrograms[{type, isVolumetric}], texelAllocator->GetTexelBlocksBufferID(), type, isVolumetric);
}

bool InitializationFeature::CreateCopyProgram(const ComRef<IShaderProgramHost> &programHost, Backend::IL::ResourceTokenType from, Backend::IL::ResourceTokenType to, bool isVolumetric) {
    return CreateProgram(programHost, copyPrograms[{from, to, isVolumetric}], texelAllocator->GetTexelBlocksBufferID(), from, to, isVolumetric);
}

bool InitializationFeature::CreateBlitPrograms(const ComRef<IShaderProgramHost> &programHost) {
    return CreateBlitProgram(programHost, Backend::IL::ResourceTokenType::Buffer, false) &&
           CreateBlitProgram(programHost, Backend::IL::ResourceTokenType::Texture, false) &&
           CreateBlitProgram(programHost, Backend::IL::ResourceTokenType::Texture, true);
}

bool InitializationFeature::CreateCopyPrograms(const ComRef<IShaderProgramHost> &programHost) {
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

FeatureInfo InitializationFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Initialization";
    info.description = "Instrumentation and validation of resource initialization prior to reads";

    // Resource bounds requires valid descriptor data, for proper safe-guarding add the descriptor feature as a dependency.
    // This ensures that during instrumentation, we are operating on the already validated, and potentially safe-guarded, descriptor data.
    info.dependencies.push_back(DescriptorFeature::kID);

    // OK
    return info;
}
