#pragma once

// Layer
#include <Backends/DX12/Allocation/Allocation.h>

// Std
#include <vector>

struct ConstantShaderDataStaging {
    /// Check if this staging buffer can accomodate for a given length
    bool CanAccomodate(size_t length) {
        return head + length <= size;
    }

    /// Underlying allocation
    Allocation allocation;

    /// Total size of this allocation
    size_t size{0};

    /// Head offset of this allocation
    size_t head{0};
};

struct ConstantShaderDataBuffer {
    /// Device visible allocation
    Allocation allocation;

    /// View to device visible allocation
    D3D12_CONSTANT_BUFFER_VIEW_DESC view{};

    /// All staging buffers
    std::vector<ConstantShaderDataStaging> staging;
};

/// Simple remapping lookup table
using ShaderConstantsRemappingTable = std::vector<uint32_t>;
