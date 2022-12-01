#include <Features/Concurrency//Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/ResourceTokenEmitter.h>
#include <Backend/CommandContext.h>

// Generated schema
#include <Schemas/Features/Concurrency.h>

// Message
#include <Message/IMessageStorage.h>

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

    // Allocate lock buffer
    //   ? Each respective PUID takes one lock integer, representing the current event id
    lockBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = 1u << Backend::IL::kResourceTokenPUIDBitCount,
        .format = Backend::IL::Format::R32UInt
    });

    // Allocate event data
    //   ? Lightweight event data
    eventID = shaderDataHost->CreateEventData(ShaderDataEventInfo { });

    // OK
    return true;
}

FeatureHookTable ConcurrencyFeature::GetHookTable() {
    FeatureHookTable table{};
    table.dispatch = BindDelegate(this, ConcurrencyFeature::OnDispatch);
    table.drawInstanced = BindDelegate(this, ConcurrencyFeature::OnDrawInstanced);
    table.drawIndexedInstanced = BindDelegate(this, ConcurrencyFeature::OnDrawIndexedInstanced);
    return table;
}

void ConcurrencyFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void ConcurrencyFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void ConcurrencyFeature::Inject(IL::Program &program) {
    // Get the data ids
    IL::ID lockBufferDataID = program.GetShaderDataMap().Get(lockBufferID)->id;
    IL::ID eventDataID = program.GetShaderDataMap().Get(eventID)->id;

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        // Instruction of interest?
        IL::ID resource;
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

        // Out of bounds block
        IL::Emitter<> oob(program, *context.function.GetBasicBlocks().AllocBlock());
        oob.AddBlockFlag(BasicBlockFlag::NoInstrumentation);

        // Export the message
        ResourceRaceConditionMessage::ShaderExport msg;
        msg.sguid = oob.UInt32(sguid);
        msg.LUID = eventDataID;
        oob.Export(exportID, msg);

        // Branch back
        oob.Branch(resumeBlock);

        // Perform instrumentation check
        IL::Emitter<> pre(program, context.basicBlock);

        // Get global id of resource
        IL::ResourceTokenEmitter token(pre, resource);

        // Compare exchange at PUID with the event
        IL::ID previousLock = pre.AtomicCompareExchange(pre.AddressOf(lockBufferDataID, token.GetPUID()), pre.UInt32(0), eventDataID);

        // Is any of the indices larger than the resource size
        IL::ID cond = pre.And(
            pre.NotEqual(previousLock, pre.UInt32(0)),
            pre.NotEqual(previousLock, eventDataID)
        );

        // If so, branch to failure, otherwise resume
        pre.BranchConditional(cond, oob.GetBasicBlock(), resumeBlock, IL::ControlFlow::Selection(resumeBlock));
        return instr;
    });
}

FeatureInfo ConcurrencyFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Concurrency";
    info.description = "Instrumentation and validation of race conditions across events or queues";
    return info;
}

void ConcurrencyFeature::OnDrawInstanced(CommandContext *context, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    // Assign next event UID, small chance of collision if any event lingers past UINT32_MAX
    context->eventStack.Set(eventID, eventCounter++);
}

void ConcurrencyFeature::OnDrawIndexedInstanced(CommandContext *context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    // Assign next event UID, small chance of collision if any event lingers past UINT32_MAX
    context->eventStack.Set(eventID, eventCounter++);
}

void ConcurrencyFeature::OnDispatch(CommandContext *context, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) {
    // Assign next event UID, small chance of collision if any event lingers past UINT32_MAX
    context->eventStack.Set(eventID, eventCounter++);
}
