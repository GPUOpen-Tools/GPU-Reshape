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
#include <Features/Initialization/SRBMaskingShaderProgram.h>
#include <Features/Descriptor/Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/ResourceTokenEmitter.h>
#include <Backend/IL/ResourceTokenType.h>
#include <Backend/CommandContext.h>
#include <Backend/Command/BufferDescriptor.h>
#include <Backend/Command/TextureDescriptor.h>
#include <Backend/Resource/ResourceInfo.h>
#include <Backend/Command/AttachmentInfo.h>
#include <Backend/Command/RenderPassInfo.h>
#include <Backend/Command/CommandBuilder.h>
#include <Backend/ShaderProgram/IShaderProgramHost.h>
#include <Backend/Scheduler/IScheduler.h>

// Generated schema
#include <Schemas/Features/Initialization.h>
#include <Schemas/Instrumentation.h>
#include <Schemas/InstrumentationCommon.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>

// Common
#include <Common/Registry.h>

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

    // Allocate lock buffer
    //   ? Each respective PUID takes one lock integer, representing the current event id
    initializationMaskBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = 1u << Backend::IL::kResourceTokenPUIDBitCount,
        .format = Backend::IL::Format::R32UInt
    });

    // Must have program host
    auto programHost = registry->Get<IShaderProgramHost>();
    if (!programHost) {
        return false;
    }

    // Create the SRB masking program
    srbMaskingShaderProgram = registry->New<SRBMaskingShaderProgram>(initializationMaskBufferID);
    if (!srbMaskingShaderProgram->Install()) {
        return false;
    }

    // Register masker
    srbMaskingShaderProgramID = programHost->Register(srbMaskingShaderProgram);

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
    builder.StageBuffer(initializationMaskBufferID, IL::kResourceTokenPUIDReservedNullBuffer * sizeof(uint32_t), sizeof(uint32_t), &fullSRB);
    builder.StageBuffer(initializationMaskBufferID, IL::kResourceTokenPUIDReservedNullTexture * sizeof(uint32_t), sizeof(uint32_t), &fullSRB);
    
    // Schedule blocking
    scheduler->Schedule(Queue::Graphics, buffer);
    scheduler->WaitForPending();

    // OK
    return true;
}

FeatureHookTable InitializationFeature::GetHookTable() {
    FeatureHookTable table{};
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

void InitializationFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const SetInstrumentationConfigMessage config = CollapseOrDefault<SetInstrumentationConfigMessage>(specialization);
    
    // Get the data ids
    IL::ID initializationMaskBufferDataID = program.GetShaderDataMap().Get(initializationMaskBufferID)->id;

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

        // Write operations just assign the new mask
        if (isWrite) {
            // Insert prior to IOI
            IL::Emitter<> emitter(program, context.basicBlock, it);

            // Get global id of resource
            IL::ResourceTokenEmitter token(emitter, resource);

            // Get token details
            IL::ID SRB = emitter.UInt32(1);
            IL::ID PUID = token.GetPUID();

            // Multiple events may write to the same resource, accumulate the SRB atomically
            const bool UseAtomics = true;

            // Atomics?
            if (UseAtomics) {
                // Or the destination resource
                emitter.AtomicOr(emitter.AddressOf(initializationMaskBufferDataID, PUID), SRB);
            } else {
                // Load buffer pointer
                IL::ID bufferID = emitter.Load(initializationMaskBufferDataID);
                
                // Get current mask
                IL::ID srbMask = emitter.Extract(emitter.LoadBuffer(bufferID, PUID), program.GetConstants().UInt(0)->id);

                // Bit-Or with resource mask
                emitter.StoreBuffer(bufferID, PUID, emitter.BitOr(srbMask, SRB));
            }
            
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

        // Get token details
        IL::ID SRB = pre.UInt32(1);
        IL::ID PUID = token.GetPUID();

        // Get the current mask
        IL::ID currentMask = pre.Extract(pre.LoadBuffer(pre.Load(initializationMaskBufferDataID), PUID), program.GetConstants().UInt(0)->id);

        // Compare mask against token SRB
        IL::ID cond = pre.NotEqual(pre.BitAnd(currentMask, SRB), SRB);

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
        }
        
        // Export the message
        mismatch.Export(exportID, msg);

        // Branch back
        mismatch.Branch(resumeBlock);
        return instr;
    });
}

void InitializationFeature::OnMapResource(const ResourceInfo &source) {
    std::lock_guard guard(mutex);

    // Mark for host initialization
    pendingInitializationQueue.push_back(InitialiationTag {
        .puid = source.token.puid,
        .srb = ~0u
    });
}

void InitializationFeature::OnCopyResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest) {
    MaskResourceSRB(context, dest.token.puid, ~0u);
}

void InitializationFeature::OnResolveResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest) {
    MaskResourceSRB(context, dest.token.puid, ~0u);
}

void InitializationFeature::OnClearResource(CommandContext* context, const ResourceInfo& resource) {
    MaskResourceSRB(context, resource.token.puid, ~0u);
}

void InitializationFeature::OnWriteResource(CommandContext* context, const ResourceInfo& resource) {
    MaskResourceSRB(context, resource.token.puid, ~0u);
}

void InitializationFeature::OnBeginRenderPass(CommandContext *context, const RenderPassInfo &passInfo) {
    // Initialize all color targets
    for (uint32_t i = 0; i < passInfo.attachmentCount; i++) {
        const AttachmentInfo& info = passInfo.attachments[i];

        // Only mark as initialized if the destination is written to
        if (info.loadAction == AttachmentAction::Clear ||
            info.storeAction == AttachmentAction::Store ||
            info.storeAction == AttachmentAction::Resolve) {
            MaskResourceSRB(context, info.resource.token.puid, ~0u);
        }

        // Always mark resolve targets as initialized
        if (info.resolveResource) {
            MaskResourceSRB(context, info.resolveResource->token.puid, ~0u);
        }
    }

    // Has depth?
    if (passInfo.depthAttachment) {
        // Only mark as initialized if the destination is written to
        if (passInfo.depthAttachment->loadAction == AttachmentAction::Clear ||
            passInfo.depthAttachment->storeAction == AttachmentAction::Store ||
            passInfo.depthAttachment->storeAction == AttachmentAction::Resolve) {
            MaskResourceSRB(context, passInfo.depthAttachment->resource.token.puid, ~0u);
        }   

        // Always mark resolve targets as initialized
        if (passInfo.depthAttachment->resolveResource) {
            MaskResourceSRB(context, passInfo.depthAttachment->resolveResource->token.puid, ~0u);
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

    // Create builder
    CommandBuilder builder(hookContexts.preContext->buffer);

    // Initialize all PUIDs
    for (const InitialiationTag& tag : pendingInitializationQueue) {
        uint32_t& initializedSRB = puidSRBInitializationMap[tag.puid];

        // May already be initialized
        if ((initializedSRB & tag.srb) == tag.srb) {
            continue;
        }

        // Mark host side initialization
        initializedSRB |= tag.srb;

        // Mark device side initialization
        builder.StageBuffer(initializationMaskBufferID, tag.puid * sizeof(uint32_t), sizeof(uint32_t), &tag.srb);
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
    if (it->second.committedInitializationHead <= committedInitializationBase) {
        return;
    }

    // Shave off all known committed initialization events
    const size_t shaveCount = it->second.committedInitializationHead - committedInitializationBase;
    pendingInitializationQueue.erase(pendingInitializationQueue.begin(), pendingInitializationQueue.begin() + shaveCount);

    // Set new base
    committedInitializationBase = it->second.committedInitializationHead;
    commandContexts.erase(contextHandle);
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

void InitializationFeature::MaskResourceSRB(CommandContext *context, uint64_t puid, uint32_t srb) {
    {
        std::lock_guard guard(mutex);
        uint32_t& initializedSRB = puidSRBInitializationMap[puid];

        // May already be initialized
        if ((initializedSRB & srb) == srb) {
            return;
        }

        // Mark host side initialization
        initializedSRB |= srb;
    }

    // Mask the entire resource as mapped
    CommandBuilder builder(context->buffer);
    builder.SetShaderProgram(srbMaskingShaderProgramID);
    builder.SetEventData(srbMaskingShaderProgram->GetPUIDEventID(), static_cast<uint32_t>(puid));
    builder.SetEventData(srbMaskingShaderProgram->GetMaskEventID(), ~0u);
    builder.Dispatch(1, 1, 1);
}
