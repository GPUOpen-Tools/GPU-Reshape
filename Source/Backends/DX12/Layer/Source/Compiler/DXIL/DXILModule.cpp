#include <Backends/DX12/Compiler/DXIL/DXILModule.h>

DXILModule::DXILModule(const Allocators &allocators) : DXILModule(allocators, new(allocators) IL::Program(allocators, 0x0)) {
    nested = false;
}

DXILModule::DXILModule(const Allocators &allocators, IL::Program *program) :
    allocators(allocators),
    program(program),
    table(allocators, *program) {
    /* */
}

DXILModule::~DXILModule() {
    if (!nested) {
        destroy(program, allocators);
    }
}

bool DXILModule::Parse(const void *byteCode, uint64_t byteLength) {
    return table.Parse(byteCode, byteLength);
}

IL::Program *DXILModule::GetProgram() {
    return program;
}
