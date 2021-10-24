#pragma once

// Layer
#include "Vulkan.h"

// Generated
#include <Backends/Vulkan/CommandBufferDispatchTable.Gen.h>

struct DeviceDispatchTable
{
    /// Populate this table
    /// \param getProcAddr the device proc address fn for the next layer
    void Populate(VkDevice device, PFN_vkGetDeviceProcAddr getProcAddr);

    /// Get the hook address for a given name
    /// \param name the name to hook
    /// \return the hooked address, may be nullptr
    static PFN_vkVoidFunction GetHookAddress(const char* name);

    /// Command buffer dispatch table
    CommandBufferDispatchTable commandBufferDispatchTable;
};
