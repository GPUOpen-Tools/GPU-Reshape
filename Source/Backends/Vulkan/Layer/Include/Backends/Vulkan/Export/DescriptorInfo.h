#pragma once

// Common
#include <Common/ReferenceObject.h>

// Layer
#include <Backends/Vulkan/Vulkan.h>

/// A single allocation descriptor allocation
struct ShaderExportSegmentDescriptorInfo {
    VkDescriptorSet set{VK_NULL_HANDLE};
    uint32_t        poolIndex{0};
};
