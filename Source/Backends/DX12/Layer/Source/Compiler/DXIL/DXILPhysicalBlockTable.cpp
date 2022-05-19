#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>

DXILPhysicalBlockTable::DXILPhysicalBlockTable(const Allocators &allocators, IL::Program &program) :
    scan(allocators),
    function(allocators, program, *this),
    type(allocators, program, *this),
    global(allocators, program, *this) {

}

bool DXILPhysicalBlockTable::Parse(const void* byteCode, uint64_t byteLength) {
    if (!scan.Scan(byteCode, byteLength)) {
        return false;
    }

    // Parse blocks
    type.Parse();
    global.Parse();
    function.Parse();

    // OK
    return true;
}

