#pragma once

// Layer
#include <Backends/Vulkan/Allocation/Allocation.h>

// Std
#include <vector>

struct ConstantShaderDataBuffer {
    /// Device visible allocation
    Allocation allocation;

    /// Buffer handle
    VkBuffer buffer{VK_NULL_HANDLE};
};

/// Simple constant lookup table
using ShaderConstantsRemappingTable = std::vector<uint32_t>;
