#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>

bool DXBCModule::Parse(const void* byteCode, uint64_t byteLength) {
    return table.Parse(byteCode, byteLength);
}
