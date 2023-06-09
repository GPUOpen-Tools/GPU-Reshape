#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/Resource/VirtualResourceMapping.h>

// Std
#include <cstdint>

// Forward declarations
struct DeviceDispatchTable;

struct ImageState {
    /// Backwards reference
    DeviceDispatchTable* table;

    /// User Image
    VkImage object{VK_NULL_HANDLE};

    /// Allocated mapping
    VirtualResourceMapping virtualMappingTemplate;

    /// Creation info
    VkImageCreateInfo createInfo;

    /// Optional, owner of this image, f.x. a swapchain
    uint64_t owningHandle{0u};

    /// Optional, debug name
    char* debugName{nullptr};

    /// Unique identifier, unique for the type
    uint64_t uid;
};

struct ImageViewState {
    /// Backwards reference
    ImageState* parent;

    /// User Image
    VkImageView object{VK_NULL_HANDLE};

    /// Allocated mapping
    VirtualResourceMapping virtualMapping;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
