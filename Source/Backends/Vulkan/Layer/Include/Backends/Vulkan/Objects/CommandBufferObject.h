#pragma once

// Layer
#include "Backends/Vulkan/CommandBufferDispatchTable.Gen.h"

// Common
#include "Common/Containers/ReferenceObject.h"

// Vulkan
#include "Backends/Vulkan/Vulkan.h"

// Std
#include <vector>

/// Forward declarations
struct DeviceDispatchTable;
struct ShaderExportStreamState;
struct CommandPoolState;

/// Wrapped command buffer object
struct CommandBufferObject {
    /// Add a referenced object to the GPU lifetime of this command buffer
    ///   ? Note, not immediate, lifetime completion is checked when queried
    /// \param obj the referenced object, once the gpu has exhausted the command buffer, the objects are released
    void AddLifetime(ReferenceObject* obj) {
        obj->AddUser();
        gpuReferences.emplace_back(obj);
    }

    /// Object dispatch
    void*                next_dispatchTable;
    VkCommandBuffer      object;
    DeviceDispatchTable* table;
    CommandPoolState*    pool;

    /// Acquired dispatch table
    CommandBufferDispatchTable dispatchTable;

    /// Current streaming state
    ShaderExportStreamState* streamState;

    /// GPU lifetime references
    std::vector<ReferenceObject*> gpuReferences;
};
