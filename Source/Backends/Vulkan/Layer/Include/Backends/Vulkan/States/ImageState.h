#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/Resource/VirtualResourceMapping.h>

// Common
#include "Common/Containers/ReferenceObject.h"

// Std
#include <cstdint>
#include <vector>

// Forward declarations
struct DeviceDispatchTable;

struct ImageState {
    /// Backwards reference
    DeviceDispatchTable* table;

    /// User Image
    VkImage object{VK_NULL_HANDLE};

    /// Allocated mapping
    VirtualResourceMapping virtualMappingTemplate;

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
