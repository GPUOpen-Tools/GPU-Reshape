#pragma once

// Ensure the beta extensions are enabled
#define VK_ENABLE_BETA_EXTENSIONS 1

// No prototypes
#define VK_NO_PROTOTYPES 1

// Vulkan
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_beta.h>

/// Get the internally stored table
template<typename T>
inline void *GetInternalTable(T inst) {
    if (!inst) {
        return nullptr;
    }

    return *(void **) inst;
}