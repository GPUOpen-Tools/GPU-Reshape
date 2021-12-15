#pragma once

// Layer
#include <Backends/Vulkan/CommandBufferDispatchTable.Gen.h>
#include "ReferenceObject.h"

// Vulkan
#include "Vulkan.h"

// Std
#include <vector>

/// Forward declarations
struct DeviceDispatchTable;
struct CommandPoolState;

/// Wrapped command buffer object
struct CommandBufferObject {
    void*                next_dispatchTable;
    VkCommandBuffer      object;
    DeviceDispatchTable* table;
    CommandPoolState*    pool;

    /// Acquired dispatch table
    CommandBufferDispatchTable dispatchTable;

    /// GPU lifetime references
    std::vector<ReferenceObject*> gpuReferences;
};
