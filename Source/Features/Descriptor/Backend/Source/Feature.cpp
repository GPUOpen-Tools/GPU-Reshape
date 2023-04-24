#include <Features/Descriptor/Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/ResourceTokenEmitter.h>
#include <Backend/IL/ResourceTokenType.h>
#include <Backend/CommandContext.h>
#include <Backend/IL/BasicBlockFlags.h>
#include <Backend/IL/ResourceTokenPacking.h>
#include <Backend/IL/BasicBlockCommon.h>
#include <Backend/IL/Constant.h>

// Generated schema
#include <Schemas/Features/Descriptor.h>
#include <Schemas/Instrumentation.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStreamCommon.h>
#include <Message/MessageStream.h>

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

void DescriptorFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Options
    const bool isSafeGuarded = FindOrDefault<SetSafeGuardMessage>(specialization, {.enabled = false}).enabled;

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext &context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Pooled resource id
        IL::ID resource;

        // Expected type pooled at runtime
        Backend::IL::ResourceTokenType compileTypeLiteral;

        // Instruction of interest?
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
            case IL::OpCode::SampleTexture:
                resource = it->As<IL::SampleTextureInstruction>()->texture;
                compileTypeLiteral = Backend::IL::ResourceTokenType::Texture;
                break;
        }

        // Do we need a merge (phi) for result data?
        const bool needsSafeGuardCFMerge = isSafeGuarded && it->result != IL::InvalidID;

        // Resulting type of the instruction (safe-guarding)
        const Backend::IL::Type* resultType{nullptr};
        if (needsSafeGuardCFMerge) {
            resultType = context.program.GetTypeMap().GetType(it->result);
        }

        // Bind the SGUID
        ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

        // Allocate resume
        IL::BasicBlock *resumeBlock = context.function.GetBasicBlocks().AllocBlock();

        // Split this basic block, move all instructions post and including the instrumented instruction to resume
        // ! iterator invalidated
        auto instr = context.basicBlock.Split(resumeBlock, isSafeGuarded ? std::next(it) : it);

        // Safe-guard identifiers, later merged with phi
        IL::ID safeGuardValue    = it->result;
        IL::ID safeGuardRedirect = IL::InvalidID;
        IL::ID safeGuardZero     = IL::InvalidID;

        // Allocate match block if safe-guarded
        IL::BasicBlock* matchBlock;
        if (isSafeGuarded) {
            matchBlock = context.function.GetBasicBlocks().AllocBlock();
            matchBlock->AddFlag(BasicBlockFlag::Visited);

            // Move offending instruction to this block
            IL::BasicBlock::Iterator splitIt = context.basicBlock.Split(matchBlock, it);

            // Redirect the user instruction
            if (needsSafeGuardCFMerge) {
                safeGuardRedirect = context.program.GetIdentifierMap().AllocID();
                Backend::IL::RedirectResult(context.program, splitIt, safeGuardRedirect);
            }

            // Branch back to resume
            IL::Emitter<> match(program, *matchBlock);
            match.Branch(resumeBlock);
        }

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
        IL::ID runtimePUID = token.GetPUID();

        // Types must match, or out of bounds
        IL::ID cond = pre.Or(
            pre.NotEqual(compileType, runtimeType),
            pre.GreaterThanEqual(runtimePUID, pre.UInt32(IL::kResourceTokenPUIDInvalidStart))
        );

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(
            cond,
            mismatch.GetBasicBlock(), isSafeGuarded ? matchBlock : resumeBlock,
            IL::ControlFlow::Selection(resumeBlock)
        );

        // Special PUIDs
        IL::ID isUndefined   = mismatch.Equal(runtimePUID, mismatch.UInt32(IL::kResourceTokenPUIDUndefined));
        IL::ID isOutOfBounds = mismatch.Equal(runtimePUID, mismatch.UInt32(IL::kResourceTokenPUIDOutOfBounds));

        // Export the message
        DescriptorMismatchMessage::ShaderExport msg;
        msg.sguid = mismatch.UInt32(sguid);
        msg.compileType = compileType;
        msg.runtimeType = runtimeType;
        msg.isUndefined = mismatch.Select(isUndefined, mismatch.UInt32(1), mismatch.UInt32(0));
        msg.isOutOfBounds = mismatch.Select(isOutOfBounds, mismatch.UInt32(1), mismatch.UInt32(0));
        mismatch.Export(exportID, msg);

        // If safe-guarded, allocate null fallback constant
        if (needsSafeGuardCFMerge) {
            safeGuardZero = context.program.GetConstants().FindConstantOrAdd(resultType, Backend::IL::NullConstant{})->id;
        }

        // Branch to resume
        mismatch.Branch(resumeBlock);

        // If safe-guarded, phi the data back together
        if (needsSafeGuardCFMerge) {
            IL::Emitter<> resumeEmitter(program, *resumeBlock, instr);

            // Select the appropriate type with phi
            resumeEmitter.Phi(
                safeGuardValue,
                matchBlock, safeGuardRedirect,
                mismatch.GetBasicBlock(), safeGuardZero
            );

            // Resume after phi
            instr = resumeEmitter.GetIterator();
        }

        // OK
        return instr;
    });
}

FeatureInfo DescriptorFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Descriptor";
    info.description = "Instrumentation and validation of descriptor usage";
    return info;
}
