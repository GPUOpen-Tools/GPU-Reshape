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

struct BufferState {
    /// Backwards reference
    DeviceDispatchTable* table;

    /// User buffer
    VkBuffer object{VK_NULL_HANDLE};

    /// Allocated mapping
    VirtualResourceMapping virtualMapping;

    /// Unique identifier, unique for the type
    uint64_t uid;
};

struct BufferViewState {
    /// Backwards reference
    BufferState* parent;

    /// User buffer
    VkBufferView object{VK_NULL_HANDLE};

    /// Allocated mapping
    VirtualResourceMapping virtualMapping;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
