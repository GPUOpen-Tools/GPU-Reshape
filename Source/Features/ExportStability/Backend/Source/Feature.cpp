#include <Features/ExportStability//Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>

// Generated schema
#include <Schemas/Features/ExportStability.h>

// Message
#include <Message/IMessageStorage.h>

// Common
#include <Common/Registry.h>

bool ExportStabilityFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<UnstableExportMessage>();

    // Optional sguid host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // OK
    return true;
}

FeatureHookTable ExportStabilityFeature::GetHookTable() {
    return FeatureHookTable{};
}

void ExportStabilityFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void ExportStabilityFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void ExportStabilityFeature::Inject(IL::Program &program) {
    // Unsigned target type
    const Backend::IL::Type* uint32Type = program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType {.bitWidth = 32, .signedness = false});

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Instruction of interest?
        IL::ID value;
        switch (it->opCode) {
            default:
                return it;
            case IL::OpCode::StoreBuffer:
                value = it->As<IL::StoreBufferInstruction>()->value;
                break;
            case IL::OpCode::StoreTexture:
                value = it->As<IL::StoreTextureInstruction>()->texel;
                break;
            case IL::OpCode::Export:
                value = it->As<IL::ExportInstruction>()->value;
                break;
        }

        // Get type
        const Backend::IL::Type* valueType = program.GetTypeMap().GetType(value);

        // Stability only instrumented against floating point
        if (!Backend::IL::IsComponentType<Backend::IL::FPType>(valueType)) {
            return it;
        }

        // Instrumentation Segmentation
        //
        //             BEFORE                                 AFTER
        //
        //   ┌─────┬─────────────┬───────┐      ┌─────┐                   ┌─────────────┬──────┐
        //   │     │             │       │      │     │        OK         │             │      │
        //   │ Pre │ Instruction │ Post  │      │ Pre ├───────────────────┤ Instruction │ Post │
        //   │     │             │       │      │     │                   │   [RESUME]  │      │
        //   └─────┴─────────────┴───────┘      └──┬──┘                   └──────┬──────┴──────┘
        //                                         │    ┌────────────┐           │
        //                                     INV │    │            │           │
        //                                         └────┤ Inf / NaN  ├───────────┘
        //                                              │            │
        //                                              └────────────┘

        // Bind the SGUID
        ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

        // Allocate resume
        IL::BasicBlock* resumeBlock = context.function.GetBasicBlocks().AllocBlock();

        // Split this basic block, move all instructions post and including the instrumented instruction to resume
        // ! iterator invalidated
        auto instr = context.basicBlock.Split(resumeBlock, it);

        // Perform instrumentation check
        IL::Emitter<> pre(program, context.basicBlock);

        // Failure conditions
        IL::ID isInf = pre.Any(pre.IsInf(value));
        IL::ID isNaN = pre.Any(pre.IsNaN(value));

        // Out of bounds block
        IL::Emitter<> oob(program, *context.function.GetBasicBlocks().AllocBlock());
        oob.AddBlockFlag(BasicBlockFlag::NoInstrumentation);

        // Export the message
        UnstableExportMessage::ShaderExport msg;
        msg.sguid = oob.UInt32(sguid);
        msg.isNaN = oob.Select(isNaN, oob.UInt32(1), oob.UInt32(0));
        oob.Export(exportID, msg);

        // Branch back
        oob.Branch(resumeBlock);

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(pre.BitOr(isInf, isNaN), oob.GetBasicBlock(), resumeBlock, IL::ControlFlow::Selection(resumeBlock));
        return instr;
    });
}


