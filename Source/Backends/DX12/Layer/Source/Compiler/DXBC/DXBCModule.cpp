#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>
#include <Backends/DX12/Config.h>

DXBCModule::DXBCModule(const Allocators &allocators, uint64_t shaderGUID, const GlobalUID &instrumentationGUID) : DXBCModule(allocators, new(allocators) IL::Program(allocators, shaderGUID), instrumentationGUID) {
    nested = false;

#if SHADER_COMPILER_DEBUG
    instrumentationGUIDName = instrumentationGUID.ToString();
#endif // SHADER_COMPILER_DEBUG
}

DXBCModule::DXBCModule(const Allocators &allocators, IL::Program *program, const GlobalUID& instrumentationGUID) :
    allocators(allocators),
    program(program),
    table(allocators, *program),
    instrumentationGUID(instrumentationGUID) {
    /* */
}

DXBCModule::~DXBCModule() {
    if (!nested) {
        destroy(program, allocators);
    }
}

DXModule *DXBCModule::Copy() {
    // Copy program
    IL::Program *programCopy = program->Copy();

    // Create module copy
    auto module = new(allocators) DXBCModule(allocators, programCopy, instrumentationGUID);

    // OK
    return module;
}

bool DXBCModule::Parse(const void *byteCode, uint64_t byteLength) {
    // Try to parse
    if (!table.Parse(byteCode, byteLength)) {
        return false;
    }

    // OK
    return true;
}

IL::Program *DXBCModule::GetProgram() {
    return program;
}

GlobalUID DXBCModule::GetInstrumentationGUID() {
    return instrumentationGUID;
}
