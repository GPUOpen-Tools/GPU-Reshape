#pragma once

// Layer
#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/States/DescriptorLayoutPhysicalMapping.h"
#include "Backends/Vulkan/DeepCopyObjects.Gen.h"

// Std
#include <cstdint>

struct DescriptorSetLayoutState {
    /// User pipeline
    VkDescriptorSetLayout object{VK_NULL_HANDLE};

    /// Physical mapping
    DescriptorLayoutPhysicalMapping physicalMapping;

    /// Hash for compatability tests
    uint64_t compatabilityHash{0};

    /// Dynamic offsets
    uint32_t dynamicOffsets{0};

    /// Number of descriptors
    uint32_t descriptorCount{0};

    /// Unique identifier, unique for the type
    uint64_t uid;
};
