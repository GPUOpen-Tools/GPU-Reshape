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

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/ResourceTokenEmitter.h>
#include <Backend/IL/ResourceTokenType.h>
#include <Backend/CommandContext.h>
#include <Backend/Resource/ResourceInfo.h>
#include <Backend/Command/AttachmentInfo.h>
#include <Backend/Command/RenderPassInfo.h>
#include <Backend/Command/CommandBuilder.h>
#include <Backend/Resource/TexelAddressAllocator.h>
#include <Backend/Resource/TexelAddressEmitter.h>
#include <Backend/ShaderProgram/IShaderProgramHost.h>
#include <Backend/Scheduler/IScheduler.h>
#include <Features/Initialization/BitIndexing.h>
#include <Features/Initialization/MaskBlitParameters.h>
#include <Features/Initialization/MaskCopyRangeParameters.h>
#include <Features/Initialization/KernelShared.h>

// Generated schema
#include <Schemas/Features/Initialization.h>
#include <Schemas/Instrumentation.h>
#include <Schemas/InstrumentationCommon.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Common
#include <Common/Registry.h>

// todo[init]: remove
static constexpr uint32_t kTempSize = 512'000'000;

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

    // Allocate puid mapping buffer
    puidMemoryBaseBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = 1u << Backend::IL::kResourceTokenPUIDBitCount,
        .format = Backend::IL::Format::R32UInt
    });

    // Allocate texel mask buffer
    // todo[init]: dynamically grown with syncs
    texelMaskBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = kTempSize,
        .format = Backend::IL::Format::R32UInt
    });

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
    // Get scheduler
    auto scheduler = registry->Get<IScheduler>();
    if (!scheduler) {
        return false;
    }

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
    scheduler->Schedule(Queue::Graphics, buffer);
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

struct TexelAddress {
    /// Source coordinates
    /// todo[init]: offset by base when reporting!
    IL::ID x{IL::InvalidID};
    IL::ID y{IL::InvalidID};
    IL::ID z{IL::InvalidID};
    IL::ID mip{IL::InvalidID};

    /// Texel address offset
    IL::ID texelOffset{IL::InvalidID};
};

static TexelAddress InjectTexelAddress(IL::Emitter<>& emitter, IL::ResourceTokenEmitter<IL::Emitter<>>& token, IL::ID resource, const IL::InstructionRef<> instr) {
    IL::Program* program = emitter.GetProgram();

    // Get type
    const Backend::IL::Type *resourceType = program->GetTypeMap().GetType(resource);

    // Number of dimensions of the resource
    uint32_t dimensions = 1;

    // Is this resource volumetric in nature? Affects address computation
    bool isVolumetric = false;

    // Determine from resource type
    if (auto texture = resourceType->Cast<Backend::IL::TextureType>()) {
        dimensions = Backend::IL::GetDimensionSize(texture->dimension);
        isVolumetric = texture->dimension == Backend::IL::TextureDimension::Texture3D;
    } else if (auto buffer = resourceType->Cast<Backend::IL::BufferType>()) {
        // Boo!
    } else {
        ASSERT(false, "Invalid type");
        return {};
    }

    // Defaults
    IL::ID zero = program->GetConstants().UInt(0)->id;

    // Addressing offsets
    TexelAddress address;
    address.x = zero;
    address.y = zero;
    address.z = zero;
    address.mip = zero;
    address.texelOffset = zero;

    // Get offsets from instruction
    switch (instr->opCode) {
        default: {
            ASSERT(false, "Invalid instruction");
            return address;
        }
        case IL::OpCode::LoadBuffer: {
            // Buffer types just return the linear index
            address.x = instr->As<IL::LoadBufferInstruction>()->index;
            break;
        }
        case IL::OpCode::StoreBuffer: {
            // Buffer types just return the linear index
            address.x = instr->As<IL::StoreBufferInstruction>()->index;
            break;
        }
        case IL::OpCode::LoadBufferRaw: {
            // Buffer types just return the linear index
            address.x = instr->As<IL::LoadBufferRawInstruction>()->index;
            break;
        }
        case IL::OpCode::StoreBufferRaw: {
            // Buffer types just return the linear index
            address.x = instr->As<IL::StoreBufferRawInstruction>()->index;
            break;
        }
        case IL::OpCode::StoreTexture: {
            auto _instr = *instr->As<IL::StoreTextureInstruction>();

            // Vectorized index?
            if (const Backend::IL::Type* indexType = program->GetTypeMap().GetType(_instr.index); indexType->Is<Backend::IL::VectorType>()) {
                if (dimensions > 0) address.x = emitter.Extract(_instr.index, program->GetConstants().UInt(0)->id);
                if (dimensions > 1) address.y = emitter.Extract(_instr.index, program->GetConstants().UInt(1)->id);
                if (dimensions > 2) address.z = emitter.Extract(_instr.index, program->GetConstants().UInt(2)->id);
            } else {
                address.x = _instr.index;
            }
            break;
        }
        case IL::OpCode::LoadTexture: {
            auto _instr = *instr->As<IL::LoadTextureInstruction>();

            // Vectorized index?
            if (const Backend::IL::Type* indexType = program->GetTypeMap().GetType(_instr.index); indexType->Is<Backend::IL::VectorType>()) {
                if (dimensions > 0) address.x = emitter.Extract(_instr.index, program->GetConstants().UInt(0)->id);
                if (dimensions > 1) address.y = emitter.Extract(_instr.index, program->GetConstants().UInt(1)->id);
                if (dimensions > 2) address.z = emitter.Extract(_instr.index, program->GetConstants().UInt(2)->id);
            } else {
                address.x = _instr.index;
            }
            
            if (_instr.mip != IL::InvalidID) {
                address.mip = _instr.mip;
            }
            break;
        }
        case IL::OpCode::SampleTexture: {
            // todo[init]: sampled offsets!
#if 0
            auto _instr = instr->As<IL::SampleTextureInstruction>();

            IL::ID coordinate = _instr->coordinate;
            if (dimensions > 0) x = emitter.Mul(emitter.Extract(coordinate, program->GetConstants().UInt(0)->id);
            if (dimensions > 1) y = emitter.Extract(coordinate, program->GetConstants().UInt(1)->id);
            if (dimensions > 2) z = emitter.Extract(coordinate, program->GetConstants().UInt(2)->id);
            
            emitter.Mul(_instr->coordinate, )
#endif
            
            return address;
        }
    }

    // Determine texel address
    Backend::IL::TexelAddressEmitter addressEmitter(emitter, token);
    address.texelOffset = addressEmitter.LocalTexelAddress(address.x, address.y, address.z, address.mip, isVolumetric);

    // OK
    return address;
}

void InitializationFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const SetInstrumentationConfigMessage config = CollapseOrDefault<SetInstrumentationConfigMessage>(specialization);
    
    // Get the data ids
    IL::ID puidMemoryBaseBufferDataID = program.GetShaderDataMap().Get(puidMemoryBaseBufferID)->id;
    IL::ID texelMaskBufferDataID      = program.GetShaderDataMap().Get(texelMaskBufferID)->id;

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Pooled resource id
        IL::ID resource;

        // Write operation?
        bool isWrite = false;

        // Instruction of interest?
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

        IL::ID zero = program.GetConstants().UInt(0)->id;

        // Write operations just assign the new mask
        if (isWrite) {
            IL::InstructionRef ref(it);
            
            // Insert prior to IOI
            IL::Emitter<> emitter(program, context.basicBlock, it);

            // Get global id of resource
            IL::ResourceTokenEmitter token(emitter, resource);

            // Get the texel address
            TexelAddress texelAddress = InjectTexelAddress(emitter, token, resource, ref);

            // Get token details
            IL::ID PUID = token.GetPUID();

            // Get the base offset of the memory
            IL::ID baseMemoryOffsetAlign32 = emitter.Extract(emitter.LoadBuffer(emitter.Load(puidMemoryBaseBufferDataID), PUID), zero);

            // Mark it as initialized
            AtomicOrTexelAddress(emitter, texelMaskBufferDataID, baseMemoryOffsetAlign32, texelAddress.texelOffset);
            
            // Resume on next
            return emitter.GetIterator();
        }

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

        // Get global id of resource
        IL::ResourceTokenEmitter token(pre, resource);

        // Get the texel address
        TexelAddress texelAddress = InjectTexelAddress(pre, token, resource, IL::InstructionRef(instr));
        
        // Get token details
        IL::ID PUID = token.GetPUID();

        // Get the base offset of the memory
        IL::ID baseMemoryOffsetAlign32 = pre.Extract(pre.LoadBuffer(pre.Load(puidMemoryBaseBufferDataID), PUID), zero);

        // Fetch the bit
        // todo[init]: To atomic, or not to atomic?
        IL::ID texelBit = ReadTexelAddress(pre, texelMaskBufferDataID, baseMemoryOffsetAlign32, texelAddress.texelOffset);

        // If the bit is not set, the texel isn't initialized
        IL::ID cond = pre.Equal(texelBit, zero);

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(cond, mismatch.GetBasicBlock(), resumeBlock, IL::ControlFlow::Selection(resumeBlock));

        // Setup message
        UninitializedResourceMessage::ShaderExport msg;
        msg.sguid = mismatch.UInt32(sguid);
        msg.LUID = PUID;

        // Detailed instrumentation?
        if (config.detail) {
            msg.chunks |= UninitializedResourceMessage::Chunk::Detail;
            msg.detail.token = token.GetPackedToken();
            msg.detail.coordinate[0] = texelAddress.x;
            msg.detail.coordinate[1] = texelAddress.y;
            msg.detail.coordinate[2] = texelAddress.z;
            msg.detail.mip = texelAddress.mip;
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

void InitializationFeature::OnCreateResource(const ResourceInfo &source) {
    std::lock_guard guard(mutex);

    // Determine number of entries needed
    uint64_t allocationSize = addressAllocator.GetAllocationSize(source);

#if 0
    auto formatSizeMinDWord = std::max<size_t>(sizeof(uint32_t), source.token.formatSize);
    allocationSize = (allocationSize + formatSizeMinDWord - 1) / formatSizeMinDWord;
#endif // 0
    
    // Create allocation
    Allocation& allocation = allocations[source.token.puid];
    allocation.base = addressAllocator.Allocate(allocationSize);
    allocation.length = allocationSize;
    ASSERT(allocation.base % 32 == 0, "Memory base not aligned to texel width (32)");

    // Get base offset
    uint64_t memoryBaseAlign32 = allocation.base / 32;
    allocation.baseAlign32 = Cast32Checked(memoryBaseAlign32);

    // todo[init]: temp
    ASSERT(allocation.baseAlign32 + allocationSize / 32 < kTempSize, "Out of temp size");

    // Mark for pending enqueue
    pendingMappingQueue.push_back(MappingTag {
        .puid = source.token.puid,
        .memoryBaseAlign32 = allocation.baseAlign32
    });
}

void InitializationFeature::OnDestroyResource(const ResourceInfo &source) {
    Allocation& allocation = allocations.at(source.token.puid);

    // todo[init]: free it!
    addressAllocator.Free();

    allocations.erase(source.token.puid);
}

void InitializationFeature::OnMapResource(const ResourceInfo &source) {
    std::lock_guard guard(mutex);

    // Mark for host initialization
    pendingInitializationQueue.push_back(InitialiationTag {
        .info = source,
        .srb = ~0u
    });
}

void InitializationFeature::OnCopyResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest) {
    CopyResourceMaskRange(context->buffer, source, dest);
}

void InitializationFeature::OnResolveResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest) {
    // todo[init]: How can we handle resolve mapping sensibly?
    BlitResourceMask(context->buffer, dest);
}

void InitializationFeature::OnClearResource(CommandContext* context, const ResourceInfo& resource) {
    BlitResourceMask(context->buffer, resource);
}

void InitializationFeature::OnWriteResource(CommandContext* context, const ResourceInfo& resource) {
    BlitResourceMask(context->buffer, resource);
}

void InitializationFeature::OnBeginRenderPass(CommandContext *context, const RenderPassInfo &passInfo) {
    // TODO: Only blit the "active" render pass region
    
    // Initialize all color targets
    for (uint32_t i = 0; i < passInfo.attachmentCount; i++) {
        const AttachmentInfo& info = passInfo.attachments[i];

        // Only mark as initialized if the destination is written to
        if (info.loadAction == AttachmentAction::Clear ||
            info.storeAction == AttachmentAction::Store ||
            info.storeAction == AttachmentAction::Resolve) {
            BlitResourceMask(context->buffer, info.resource);
        }

        // Always mark resolve targets as initialized
        if (info.resolveResource) {
            BlitResourceMask(context->buffer, *info.resolveResource);
        }
    }

    // Has depth?
    if (passInfo.depthAttachment) {
        // Only mark as initialized if the destination is written to
        if (passInfo.depthAttachment->loadAction == AttachmentAction::Clear ||
            passInfo.depthAttachment->storeAction == AttachmentAction::Store ||
            passInfo.depthAttachment->storeAction == AttachmentAction::Resolve) {
            BlitResourceMask(context->buffer, passInfo.depthAttachment->resource);
        }   

        // Always mark resolve targets as initialized
        if (passInfo.depthAttachment->resolveResource) {
            BlitResourceMask(context->buffer, *passInfo.depthAttachment->resolveResource);
        }
    }
}

void InitializationFeature::OnSubmitBatchBegin(const SubmitBatchHookContexts& hookContexts, const CommandContextHandle *contexts, uint32_t contextCount) {
    std::lock_guard guard(mutex);

    // Not interested in empty submissions
    if (!contextCount) {
        return;
    }
    
    // Set commit base
    // Note: We track on the first context, once the first context has completed, all the rest have
    CommandContextInfo& info = commandContexts[contexts[0]];
    info.committedInitializationHead = committedInitializationBase + pendingInitializationQueue.size();
    info.committedMappingHead = committedMappingBase + pendingMappingQueue.size();

    // Create builder
    CommandBuilder builder(hookContexts.preContext->buffer);

    // Map all PUIDs
    for (const MappingTag& tag : pendingMappingQueue) {
        builder.StageBuffer(puidMemoryBaseBufferID, tag.puid * sizeof(uint32_t), sizeof(uint32_t), &tag.memoryBaseAlign32);
    }

    // Initialize all PUIDs
    for (const InitialiationTag& tag : pendingInitializationQueue) {
        uint32_t& initializedSRB = puidSRBInitializationMap[tag.info.token.puid];

        // May already be initialized
        if ((initializedSRB & tag.srb) == tag.srb) {
            continue;
        }

        // Mark host side initialization
        initializedSRB |= tag.srb;

        // Mark device side initialization
        BlitResourceMask(hookContexts.preContext->buffer, tag.info);
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
        // Shave off all known committed initialization events
        const size_t shaveCount = it->second.committedInitializationHead - committedInitializationBase;
        pendingInitializationQueue.erase(pendingInitializationQueue.begin(), pendingInitializationQueue.begin() + shaveCount);

        // Set new base
        committedInitializationBase = it->second.committedInitializationHead;
    }
    
    // If the head is less than the current base, it's already been shaved off
    if (it->second.committedMappingHead > committedMappingBase) {
        // Shave off all known committed initialization events
        const size_t shaveCount = it->second.committedMappingHead - committedMappingBase;
        pendingMappingQueue.erase(pendingMappingQueue.begin(), pendingMappingQueue.begin() + shaveCount);

        // Set new base
        committedMappingBase = it->second.committedMappingHead;
    }

    // Get rid of context
    commandContexts.erase(contextHandle);
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
    const ResourceProgram<MaskBlitShaderProgram>& program = blitPrograms[{info.token.type, info.isVolumetric}];
    
    // todo[init]: cpu side copy check
    const Allocation& allocation = allocations.at(info.token.puid);

    // Setup blit parameters
    MaskBlitParameters params{};
    params.memoryBaseElementAlign32 = allocation.baseAlign32;

    // Buffers are linearly indexed
    if (info.token.type == Backend::IL::ResourceTokenType::Buffer) {
        ASSERT(!info.bufferDescriptor.placedDescriptor, "Blitting with placement resource");
        ASSERT(info.bufferDescriptor.width <= allocation.length, "Length of of bounds");
        ASSERT(info.bufferDescriptor.offset + info.bufferDescriptor.width <= allocation.base + allocation.length, "End offset of of bounds");

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
    const ResourceProgram<MaskCopyRangeShaderProgram>& program = copyPrograms[{source.token.type, dest.token.type, source.isVolumetric}];
    
    // todo[init]: cpu side copy check
    const Allocation& sourceAllocation = allocations.at(source.token.puid);
    const Allocation& destAllocation = allocations.at(dest.token.puid);

    // Setup blit parameters
    MaskCopyRangeParameters params{};
    params.sourceMemoryBaseElementAlign32 = sourceAllocation.baseAlign32;
    params.destMemoryBaseElementAlign32 = destAllocation.baseAlign32;
    
    // Buffers are linearly indexed
    if (source.token.type == Backend::IL::ResourceTokenType::Buffer) {
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
    const ResourceProgram<MaskCopyRangeShaderProgram>& program = copyPrograms[{source.token.type, dest.token.type, isVolumetric}];
    
    // todo[init]: cpu side copy check
    const Allocation& sourceAllocation = allocations.at(source.token.puid);
    const Allocation& destAllocation = allocations.at(dest.token.puid);

    // Setup blit parameters
    MaskCopyRangeParameters params{};
    params.sourceMemoryBaseElementAlign32 = sourceAllocation.baseAlign32;
    params.destMemoryBaseElementAlign32 = destAllocation.baseAlign32;
    
    // Buffer -> Texture
    if (source.token.type == Backend::IL::ResourceTokenType::Buffer) {
        ASSERT(source.bufferDescriptor.width <= sourceAllocation.length, "Length of of bounds");
        ASSERT(source.bufferDescriptor.offset + source.bufferDescriptor.width <= sourceAllocation.base + sourceAllocation.length, "End offset of of bounds");

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
    return CreateProgram(programHost, blitPrograms[{type, isVolumetric}], texelMaskBufferID, type, isVolumetric);
}

bool InitializationFeature::CreateCopyProgram(const ComRef<IShaderProgramHost> &programHost, Backend::IL::ResourceTokenType from, Backend::IL::ResourceTokenType to, bool isVolumetric) {
    return CreateProgram(programHost, copyPrograms[{from, to, isVolumetric}], texelMaskBufferID, from, to, isVolumetric);
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
