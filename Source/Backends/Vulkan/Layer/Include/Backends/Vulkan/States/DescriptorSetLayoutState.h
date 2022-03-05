#pragma once

// Layer
#include "Backends/Vulkan/Vulkan.h"

// Std
#include <cstdint>

struct DescriptorSetLayoutState {
    /// User pipeline
    VkDescriptorSetLayout object{VK_NULL_HANDLE};

    /// Hash for compatability tests
    uint64_t compatabilityHash{0};

    /// Dynamic offsets
    uint32_t dynamicOffsets{0};

    /// Unique identifier, unique for the type
    uint64_t uid;
};
