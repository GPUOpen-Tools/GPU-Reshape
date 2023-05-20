#pragma once

// Layer
#include <Backends/DX12/Allocation/Allocation.h>

// Std
#include <vector>

struct ConstantShaderDataBuffer {
    /// Device visible allocation
    Allocation allocation;

    /// View to device visible allocation
    D3D12_CONSTANT_BUFFER_VIEW_DESC view{};
};

/// Simple remapping lookup table
using ShaderConstantsRemappingTable = std::vector<uint32_t>;
