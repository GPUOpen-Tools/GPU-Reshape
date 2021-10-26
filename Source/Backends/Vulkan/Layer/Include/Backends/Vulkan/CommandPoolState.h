#pragma once

// Vulkan
#include "Vulkan.h"

// Std
#include <vector>

/// Prototypes
struct CommandBufferObject;

/// Command pool state, not wrapped
struct CommandPoolState
{
    DeviceDispatchTable* table;

    /// Allocated command buffers
    std::vector<CommandBufferObject*> commandBuffers;
};
