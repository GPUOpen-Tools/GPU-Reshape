#pragma once

// Layer
#include <Backends/DX12/Compiler/DXModule.h>
#include <Backends/DX12/Compiler/DXIL/DXILPhysicalBlockTable.h>

/// DXIL bytecode module
class DXILModule final : public DXModule {
public:
    /// Overrides
    bool Parse(const void* byteCode, uint64_t byteLength);

private:
    /// All physical blocks
    DXILPhysicalBlockTable table;
};
