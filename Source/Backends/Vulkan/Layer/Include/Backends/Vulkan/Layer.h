#pragma once

// Vulkan
#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan_core.h>

// TODO: Exactly what is the right way of registering new extension types?
static constexpr VkStructureType VK_STRUCTURE_TYPE_GPUOPEN_GPUVALIDATION_CREATE_INFO = static_cast<VkStructureType>(1100000001);

// Forward declarations
class Registry;

/// Optional creation info for GPU Validation
struct VkGPUOpenGPUValidationCreateInfo {
    /// Structure type, must be VK_STRUCTURE_TYPE_GPUOPEN_GPUVALIDATION_CREATE_INFO
    VkStructureType sType;

    /// Next extension
    const void *pNext;

    /// Shared registry
    Registry* registry;
};
