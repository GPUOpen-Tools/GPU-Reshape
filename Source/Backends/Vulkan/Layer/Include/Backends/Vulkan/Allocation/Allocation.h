#pragma once

// Layer
#include <Backends/Vulkan/VMA.h>

/// A single allocation
struct Allocation {
    VmaAllocation     allocation{VK_NULL_HANDLE};
    VmaAllocationInfo info{};
};
