#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>

bool DXILPhysicalBlockTable::Parse(const void* byteCode, uint64_t byteLength) {
    if (!scan.Scan(byteCode, byteLength)) {
        return false;
    }

    // Parse blocks
    function.Parse();

    // OK
    return true;
}

