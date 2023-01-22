#include <Features/Initialization/Feature.h>
#include <Features/Initialization/SRBMaskingShaderProgram.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/ResourceTokenEmitter.h>
#include <Backend/IL/ResourceTokenType.h>
#include <Backend/CommandContext.h>
#include <Backend/Command/BufferDescriptor.h>
#include <Backend/Command/CommandBuilder.h>
#include <Backend/ShaderProgram/IShaderProgramHost.h>

// Generated schema
#include <Schemas/Features/Initialization.h>

// Message
#include <Message/IMessageStorage.h>

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

FeatureHookTable InitializationFeature::GetHookTable() {
    FeatureHookTable table{};
    table.copyBuffer = BindDelegate(this, InitializationFeature::OnCopyBuffer);
    return table;
}

void InitializationFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void InitializationFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void InitializationFeature::Inject(IL::Program &program) {
    // Get the data ids
    IL::ID initializationMaskBufferDataID = program.GetShaderDataMap().Get(initializationMaskBufferID)->id;

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Pooled resource id
        IL::ID resource;

        // Instruction of interest?
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::LoadBuffer:
                resource = it->As<IL::LoadBufferInstruction>()->buffer;
                break;
            case IL::OpCode::StoreBuffer:
                resource = it->As<IL::StoreBufferInstruction>()->buffer;
                break;
            case IL::OpCode::StoreTexture:
                resource = it->As<IL::StoreTextureInstruction>()->texture;
                break;
            case IL::OpCode::LoadTexture:
                resource = it->As<IL::LoadTextureInstruction>()->texture;
                break;
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
        IL::ID SRB = token.GetSRB();
        IL::ID PUID = token.GetPUID();

        // Get the current mask
        IL::ID currentMask = pre.Extract(pre.LoadBuffer(pre.Load(initializationMaskBufferDataID), token.GetPUID()), 0u);

        // Compare mask against token SRB
        IL::ID cond = pre.NotEqual(pre.BitAnd(currentMask, SRB), SRB);

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(cond, mismatch.GetBasicBlock(), resumeBlock, IL::ControlFlow::Selection(resumeBlock));

        // Export the message
        UninitializedResourceMessage::ShaderExport msg;
        msg.sguid = mismatch.UInt32(sguid);
        msg.LUID = PUID;
        mismatch.Export(exportID, msg);

        // Branch back
        mismatch.Branch(resumeBlock);
        return instr;
    });
}

void InitializationFeature::OnCopyBuffer(CommandContext* context, const BufferDescriptor& source, const BufferDescriptor& dest, uint64_t byteSize) {
    MaskResourceSRB(context, dest.token.puid, ~0u);
}

FeatureInfo InitializationFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Initialization";
    info.description = "Instrumentation and validation of resource initialization prior to reads";
    return info;
}

void InitializationFeature::MaskResourceSRB(CommandContext *context, uint64_t puid, uint32_t srb) {
    uint32_t& initializedSRB = puidSRBInitializationMap[puid];

    // May already be initialized
    if ((initializedSRB & srb) == srb) {
        return;
    }

    // Mark host side initialization
    initializedSRB |= srb;

    // Mask the entire resource as mapped
    CommandBuilder builder(context->buffer);
    builder.SetShaderProgram(srbMaskingShaderProgramID);
    builder.SetEventData(srbMaskingShaderProgram->GetPUIDEventID(), static_cast<uint32_t>(puid));
    builder.SetEventData(srbMaskingShaderProgram->GetMaskEventID(), ~0u);
    builder.Dispatch(1, 1, 1);
}
