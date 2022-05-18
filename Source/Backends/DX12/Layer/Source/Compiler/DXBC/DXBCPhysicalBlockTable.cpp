#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>

// DXIL Extension
#include <Backends/DX12/Compiler/DXIL/DXILModule.h>

// Common
#include <Common/Allocators.h>

bool DXBCPhysicalBlockTable::Parse(const void* byteCode, uint64_t byteLength) {
    if (!scan.Scan(byteCode, byteLength)) {
        return false;
    }

    // Parse blocks
    shader.Parse();

    // DXIL?
    if (DXBCPhysicalBlock* block = scan.GetPhysicalBlock(DXBCPhysicalBlockType::DXIL)) {
        dxilModule = new (Allocators{}) DXILModule;

        // Attempt to parse the module
        if (!dxilModule->Parse(block->ptr + sizeof(DXBCChunkHeader), block->length)) {
            return false;
        }
    }

    // OK
    return true;
}

