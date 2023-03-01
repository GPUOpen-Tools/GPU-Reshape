#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>
#include <Backends/DX12/Config.h>
#include <Backends/DX12/Compiler/Tags.h>

// Common
#include <Common/FileSystem.h>

// Std
#include <fstream>

DXBCModule::DXBCModule(const Allocators &allocators, uint64_t shaderGUID, const GlobalUID &instrumentationGUID) : DXBCModule(allocators, new(allocators, kAllocModuleILProgram) IL::Program(allocators, shaderGUID), instrumentationGUID) {
    nested = false;

#if SHADER_COMPILER_DEBUG
    instrumentationGUIDName = instrumentationGUID.ToString();
#endif // SHADER_COMPILER_DEBUG
}

DXBCModule::DXBCModule(const Allocators &allocators, IL::Program *program, const GlobalUID& instrumentationGUID) :
    allocators(allocators),
    table(allocators, *program),
    program(program),
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
    auto module = new(allocators, kAllocModuleDXIL) DXBCModule(allocators, programCopy, instrumentationGUID);
    module->nested = nested;

    // Copy table
    table.CopyTo(module->table);

    // OK
    return module;
}

bool DXBCModule::Parse(const void *byteCode, uint64_t byteLength) {
#if SHADER_COMPILER_DEBUG_FILE
    std::ifstream in(GetIntermediateDebugPath() / "scan.dxbc", std::ios_base::binary);

    // Get file size
    in.seekg(0, std::ios_base::end);
    uint64_t size = in.tellg();
    in.seekg(0);

    // Stream data in
    std::vector<char> data(size);
    in.read(data.data(), size);

    // Set new data
    byteCode = data.data();
    byteLength = data.size();
#endif // SHADER_COMPILER_DEBUG_FILE

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

bool DXBCModule::Compile(const DXJob& job, DXStream& out) {
    // Try to recompile for the given job
    if (!table.Compile(job)) {
        return false;
    }

    // Stitch to the program
    table.Stitch(job, out);

    // OK!
    return true;
}

IDXDebugModule *DXBCModule::GetDebug() {
    return table.debugModule;
}
