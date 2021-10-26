#pragma once

// Vulkan
#include "Vulkan.h"

/// Forward declarations
struct DeviceDispatchTable;

/// Wrapped command buffer object
struct CommandBufferObject
{
    void*                next_dispatchTable;
    VkCommandBuffer      object;
    DeviceDispatchTable* table;
};
