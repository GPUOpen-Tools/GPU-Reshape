#include <Features/ResourceBounds/Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>

// Generated schema
#include <Schemas/Features/ResourceBounds.h>

// Message
#include <Message/IMessageStorage.h>

// Common
#include <Common/Registry.h>

bool ResourceBoundsFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<ResourceIndexOutOfBoundsMessage>();

    // Optional sguid host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // OK
    return true;
}

FeatureHookTable ResourceBoundsFeature::GetHookTable() {
    return FeatureHookTable{};
}

void ResourceBoundsFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void ResourceBoundsFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void ResourceBoundsFeature::Inject(IL::Program &program) {
    // Unsigned target type
    const Backend::IL::Type* uint32Type = program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType {.bitWidth = 32, .signedness = false});

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        bool isTexture;
        bool isWrite;

        // Instruction of interest?
        switch (it->opCode) {
            default:
                return it;

            /* Handled cases */
            case IL::OpCode::StoreBuffer:
                isWrite = true;
                isTexture = false;
                break;
            case IL::OpCode::LoadBuffer:
                isWrite = false;
                isTexture = false;
                break;
            case IL::OpCode::StoreTexture:
                isWrite = true;
                isTexture = true;
                break;
            case IL::OpCode::LoadTexture:
                isWrite = false;
                isTexture = true;
                break;
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
        //                                         │    ┌───────────────┐        │
        //                                     OOB │    │               │        │
        //                                         └────┤ Out of Bounds ├────────┘
        //                                              │     [OOB]     │
        //                                              └───────────────┘

        // Bind the SGUID
        ShaderSGUID sguid = sguidHost ? sguidHost->Bind(program, it) : InvalidShaderSGUID;

        // Allocate resume
        IL::BasicBlock* resumeBlock = context.function.GetBasicBlocks().AllocBlock();

        // Split this basic block, move all instructions post and including the instrumented instruction to resume
        // ! iterator invalidated
        auto instr = context.basicBlock.Split(resumeBlock, it);

        // Out of bounds block
        IL::Emitter<> oob(program, *context.function.GetBasicBlocks().AllocBlock());
        oob.AddBlockFlag(BasicBlockFlag::NoInstrumentation);

        // Export the message
        ResourceIndexOutOfBoundsMessage::ShaderExport msg;
        msg.sguid = oob.UInt32(sguid);
        msg.isTexture = oob.UInt32(isTexture);
        msg.isWrite = oob.UInt32(isWrite);
        oob.Export(exportID, msg);

        // Branch back
        oob.Branch(resumeBlock);

        // Perform instrumentation check
        IL::Emitter<> pre(program, context.basicBlock);

        // Is any of the indices larger than the resource size
        IL::ID cond{};
        switch (instr->opCode) {
            default:
                ASSERT(false, "Unexpected opcode");
                break;
            case IL::OpCode::StoreBuffer: {
                auto* storeBuffer = instr->As<IL::StoreBufferInstruction>();
                cond = pre.Any(pre.GreaterThanEqual(pre.BitCast(storeBuffer->index, SplatToValue(program, uint32Type, storeBuffer->index)), pre.ResourceSize(storeBuffer->buffer)));
                break;
            }
            case IL::OpCode::LoadBuffer: {
                auto* loadBuffer = instr->As<IL::LoadBufferInstruction>();
                cond = pre.Any(pre.GreaterThanEqual(pre.BitCast(loadBuffer->index, SplatToValue(program, uint32Type, loadBuffer->index)), pre.ResourceSize(loadBuffer->buffer)));
                break;
            }
            case IL::OpCode::StoreTexture: {
                auto* storeTexture = instr->As<IL::StoreTextureInstruction>();
                cond = pre.Any(pre.GreaterThanEqual(pre.BitCast(storeTexture->index, SplatToValue(program, uint32Type, storeTexture->index)), pre.ResourceSize(storeTexture->texture)));
                break;
            }
            case IL::OpCode::LoadTexture: {
                auto* loadTexture = instr->As<IL::LoadTextureInstruction>();
                cond = pre.Any(pre.GreaterThanEqual(pre.BitCast(loadTexture->index, SplatToValue(program, uint32Type, loadTexture->index)), pre.ResourceSize(loadTexture->texture)));
                break;
            }
        }

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(cond, oob.GetBasicBlock(), resumeBlock, IL::ControlFlow::Selection(resumeBlock));
        return instr;
    });
}

FeatureInfo ResourceBoundsFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Resource Bounds";
    info.description = "Instrumentation and validation of resource indexing operations";
    return info;
}


