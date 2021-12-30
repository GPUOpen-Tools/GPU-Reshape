#pragma once

// Vulkan
#include <Backends/Vulkan/Vulkan.h>

// Std
#include <vector>

/// Prototypes
struct CommandBufferObject;

/// Command pool state, not wrapped
struct CommandPoolState {
    /// Allocated command buffers
    std::vector<CommandBufferObject*> commandBuffers;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
