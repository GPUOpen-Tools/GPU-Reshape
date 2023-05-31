#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>

// Forward declarations
struct DeviceDispatchTable;

struct DescriptorUpdateTemplateState {
    /// Backwards reference
    DeviceDispatchTable* table;

    /// User template
    VkDescriptorUpdateTemplate object{VK_NULL_HANDLE};

    // Creation info
    VkDescriptorUpdateTemplateCreateInfoDeepCopy createInfo;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
