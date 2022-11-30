#include <Features/Descriptor/Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/ResourceTokenEmitter.h>
#include <Backend/IL/ResourceTokenType.h>
#include <Backend/CommandContext.h>

// Generated schema
#include <Schemas/Features/Descriptor.h>

// Message
#include <Message/IMessageStorage.h>

// Common
#include <Common/Registry.h>

bool DescriptorFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<DescriptorMismatchMessage>();

    // Optional SGUID host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // OK
    return true;
}

FeatureHookTable DescriptorFeature::GetHookTable() {
    FeatureHookTable table{};
    return table;
}

void DescriptorFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void DescriptorFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void DescriptorFeature::Inject(IL::Program &program) {
    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Pooled resource id
        IL::ID resource;

        // Expected type pooled at runtime
        Backend::IL::ResourceTokenType compileTypeLiteral;

        // Instruction of interest?
        // TODO: Expose samplers!
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::LoadBuffer:
                resource = it->As<IL::LoadBufferInstruction>()->buffer;
                compileTypeLiteral = Backend::IL::ResourceTokenType::Buffer;
                break;
            case IL::OpCode::StoreBuffer:
                resource = it->As<IL::StoreBufferInstruction>()->buffer;
                compileTypeLiteral = Backend::IL::ResourceTokenType::Buffer;
                break;
            case IL::OpCode::StoreTexture:
                resource = it->As<IL::StoreTextureInstruction>()->texture;
                compileTypeLiteral = Backend::IL::ResourceTokenType::Texture;
                break;
            case IL::OpCode::LoadTexture:
                resource = it->As<IL::LoadTextureInstruction>()->texture;
                compileTypeLiteral = Backend::IL::ResourceTokenType::Texture;
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

        // Get the types
        IL::ID compileType = pre.UInt32(static_cast<uint32_t>(compileTypeLiteral));
        IL::ID runtimeType = token.GetType();

        // Types must match
        IL::ID cond = pre.NotEqual(compileType, runtimeType);

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(cond, mismatch.GetBasicBlock(), resumeBlock, IL::ControlFlow::Selection(resumeBlock));

        // Export the message
        DescriptorMismatchMessage::ShaderExport msg;
        msg.sguid = mismatch.UInt32(sguid);
        msg.compileType = compileType;
        msg.runtimeType = runtimeType;
        mismatch.Export(exportID, msg);

        // Branch back
        mismatch.Branch(resumeBlock);
        return instr;
    });
}

FeatureInfo DescriptorFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Descriptor";
    info.description = "Instrumentation and validation of descriptor usage";
    return info;
}
