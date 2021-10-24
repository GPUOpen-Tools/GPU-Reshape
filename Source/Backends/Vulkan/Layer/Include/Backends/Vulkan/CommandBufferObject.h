#pragma once

#include "Vulkan.h"

/// Forward declarations
struct DeviceDispatchTable;

/// Wrapped command buffer object
struct CommandBufferObject
{
    VkCommandBuffer      object;
    DeviceDispatchTable* table;
};
