#pragma once

// Layer
#include <Backends/Vulkan/Allocation/MirrorAllocation.h>

// Std
#include <vector>

struct DescriptorDataSegmentEntry {
    /// Allocation of this segment entry
    MirrorAllocation allocation;

    /// Device handle
    VkBuffer bufferDevice{VK_NULL_HANDLE};

    /// Host handle
    VkBuffer bufferHost{VK_NULL_HANDLE};

    /// Logical size
    uint64_t width{0};
};

struct DescriptorDataSegment {
    /// All entries within this segment
    std::vector<DescriptorDataSegmentEntry> entries;
};
