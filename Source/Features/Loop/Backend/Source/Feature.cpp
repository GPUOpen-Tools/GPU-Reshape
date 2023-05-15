#include "Backend/IL/ID.h"
#include "Backend/IL/Instruction.h"
#include <Features/Loop/Feature.h>
#include <Features/Descriptor/Feature.h>

// Backend
#include <Backend/IShaderExportHost.h>
#include <Backend/IShaderSGUIDHost.h>
#include <Backend/IL/Visitor.h>
#include <Backend/IL/TypeCommon.h>
#include <Backend/IL/ResourceTokenEmitter.h>
#include <Backend/CommandContext.h>

// Generated schema
#include <Schemas/Features/Loop.h>

// Message
#include <Message/IMessageStorage.h>

// Common
#include <Common/Registry.h>
#include <Common/Sink.h>

bool LoopFeature::Install() {
    // Must have the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Allocate the shared export
    exportID = exportHost->Allocate<LoopTerminationMessage>();

    // Optional SGUID host
    sguidHost = registry->Get<IShaderSGUIDHost>();

    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Allocate lock buffer
    //   ? Each respective PUID takes one lock integer, representing the current event id
    terminationBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
        .elementCount = kMaxTrackedSubmissions,
        .format = Backend::IL::Format::R32UInt
    });

    // OK
    return true;
}

FeatureHookTable LoopFeature::GetHookTable() {
    FeatureHookTable table{};
    return table;
}

void LoopFeature::CollectExports(const MessageStream &exports) {
    stream.Append(exports);
}

void LoopFeature::CollectMessages(IMessageStorage *storage) {
    storage->AddStreamAndSwap(stream);
}

void LoopFeature::Inject(IL::Program &program, const MessageStreamView<> &specialization) {
    // Get the data ids
    IL::ID terminationBufferDataID = program.GetShaderDataMap().Get(terminationBufferID)->id;
    GRS_SINK(terminationBufferDataID);

    // Visit all instructions
    IL::VisitUserInstructions(program, [&](IL::VisitContext& context, IL::BasicBlock::Iterator it) -> IL::BasicBlock::Iterator {
        auto* instr = it->Cast<IL::BranchConditionalInstruction>();

        // Instruction of interest?
        // Only interested in _continue control flow
        if (!instr || instr->controlFlow._continue == IL::InvalidID) {
            return it;
        }

        // TODO: ...
        return it;
    });
}

FeatureInfo LoopFeature::GetInfo() {
    FeatureInfo info;
    info.name = "Loop";
    info.description = "Instrumentation and validation of infinite loops";
    return info;
}
