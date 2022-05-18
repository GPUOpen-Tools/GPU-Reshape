#include <Backends/DX12/Compiler/DXIL/DXILModule.h>

bool DXILModule::Parse(const void* byteCode, uint64_t byteLength) {
    return table.Parse(byteCode, byteLength);
}
