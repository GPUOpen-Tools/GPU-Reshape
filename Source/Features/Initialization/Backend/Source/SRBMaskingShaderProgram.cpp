#include <Features/Initialization/SRBMaskingShaderProgram.h>

// Backend
#include <Backend/IL/ProgramCommon.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/Emitter.h>

// Common
#include <Common/Registry.h>

SRBMaskingShaderProgram::SRBMaskingShaderProgram(ShaderDataID initializationMaskBufferID) : initializationMaskBufferID(initializationMaskBufferID) {

}

bool SRBMaskingShaderProgram::Install() {
    // Shader data host
    shaderDataHost = registry->Get<IShaderDataHost>();

    // Create event data
    maskEventID = shaderDataHost->CreateEventData(ShaderDataEventInfo{});
    puidEventID = shaderDataHost->CreateEventData(ShaderDataEventInfo{});

    // OK
    return true;
}

void SRBMaskingShaderProgram::Inject(IL::Program &program) {
    // Must have termination block
    IL::BasicBlock* basicBlock = Backend::IL::GetTerminationBlock(program);
    if (!basicBlock) {
        return;
    }

    // Get the initialization buffer
    IL::ID initializationMaskBufferDataID = program.GetShaderDataMap().Get(initializationMaskBufferID)->id;
    IL::ID maskEventDataID = program.GetShaderDataMap().Get(maskEventID)->id;
    IL::ID puidEventDataID = program.GetShaderDataMap().Get(puidEventID)->id;

    // Append prior terminator
    IL::Emitter<> emitter(program, *basicBlock, basicBlock->GetTerminator());

    // Load buffer pointer
    IL::ID bufferID = emitter.Load(initializationMaskBufferDataID);

    // Get current mask
    IL::ID srbMask = emitter.Extract(emitter.LoadBuffer(bufferID, puidEventDataID), 0u);

    // Bit-Or with desired mask
    emitter.StoreBuffer(bufferID, puidEventDataID, emitter.BitOr(srbMask, maskEventDataID));
}
