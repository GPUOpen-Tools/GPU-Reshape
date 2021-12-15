#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>

// Backend
#include <Backend/ShaderExport.h>

struct ShaderExportStream {
    ShaderExportID id = InvalidShaderExportID;

    /// Counter separated for residency management
    VkBuffer counter{VK_NULL_HANDLE};

    /// Data separated for residency management
    VkBuffer data{VK_NULL_HANDLE};
};
