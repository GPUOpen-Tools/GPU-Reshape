#pragma once

// Layer
#include <Backends/DX12/Compiler/DXModule.h>
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockTable.h>

/// DXBC bytecode module
class DXBCModule final : public DXModule {
public:
    /// Overrides
    bool Parse(const void* byteCode, uint64_t byteLength);

private:
    /// Physical table
    DXBCPhysicalBlockTable table;
};
