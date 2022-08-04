#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>

// DXIL Extension
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>

// Common
#include <Common/Allocators.h>

DXBCPhysicalBlockTable::DXBCPhysicalBlockTable(const Allocators &allocators, IL::Program &program) :
    allocators(allocators),
    program(program),
    scan(allocators),
    shader(allocators, program, *this),
    pipelineStateValidation(allocators, program, *this) {
    /* */
}

bool DXBCPhysicalBlockTable::Parse(const void *byteCode, uint64_t byteLength) {
    if (!scan.Scan(byteCode, byteLength)) {
        return false;
    }

    // Parse blocks
    shader.Parse();
    pipelineStateValidation.Parse();

    // DXIL?
    if (DXBCPhysicalBlock *block = scan.GetPhysicalBlock(DXBCPhysicalBlockType::DXIL)) {
        dxilModule = new(Allocators{}) DXILModule(allocators, &program);

        // Attempt to parse the module
        if (!dxilModule->Parse(block->ptr, block->length)) {
            return false;
        }
    }

    // OK
    return true;
}

bool DXBCPhysicalBlockTable::Compile(const DXJob &job) {
    // Compile validation
    pipelineStateValidation.Compile();

    // DXIL?
    if (dxilModule) {
        DXBCPhysicalBlock *block = scan.GetPhysicalBlock(DXBCPhysicalBlockType::DXIL);

        // Attempt to compile contained module
        if (!dxilModule->Compile(job, block->stream)) {
            return false;
        }
    }

    // OK
    return true;
}

void DXBCPhysicalBlockTable::Stitch(const DXJob& job, DXStream &out) {
    scan.Stitch(job, out);
}

void DXBCPhysicalBlockTable::CopyTo(DXBCPhysicalBlockTable &out) {
    scan.CopyTo(out.scan);

    // Copy blocks
    pipelineStateValidation.CopyTo(out.pipelineStateValidation);

    // Copy submodule if present
    if (dxilModule) {
        out.dxilModule = new(Allocators{}) DXILModule(allocators, &out.program);

        // Copy to new module
        dxilModule->CopyTo(out.dxilModule);
    }
}
