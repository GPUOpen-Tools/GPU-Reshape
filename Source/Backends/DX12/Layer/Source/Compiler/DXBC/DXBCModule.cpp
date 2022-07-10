#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>

DXBCModule::DXBCModule(const Allocators &allocators) : DXBCModule(allocators, new(allocators) IL::Program(allocators, 0x0)) {
    nested = false;
}

DXBCModule::DXBCModule(const Allocators &allocators, IL::Program *program) :
    allocators(allocators),
    program(program),
    table(allocators, *program) {
    /* */
}

DXBCModule::~DXBCModule() {
    if (!nested) {
        destroy(program, allocators);
    }
}

DXModule* DXBCModule::Copy() {
    // Copy program
    IL::Program* programCopy = program->Copy();

    // Create module copy
    auto module = new (allocators) DXBCModule(allocators, programCopy);

    // OK
    return module;
}

bool DXBCModule::Parse(const void *byteCode, uint64_t byteLength) {
    return table.Parse(byteCode, byteLength);
}

IL::Program *DXBCModule::GetProgram() {
    return program;
}
