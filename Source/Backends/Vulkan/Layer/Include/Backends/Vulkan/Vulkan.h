#pragma once

// Layer
#include "Common.h"

/// Ensure the beta extensions are enabled
#define VK_ENABLE_BETA_EXTENSIONS 1

/// No prototypes
#define VK_NO_PROTOTYPES 1

/// Diagnostic logging
#ifndef NDEBUG
#   define LOG_ALLOCATION
#endif

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

/// Get the internally stored table
template<typename T>
inline void *&GetInternalTableRef(T inst) {
    return *(void **) inst;
}


/// Get the internally stored table
template<typename T, typename U>
inline void PatchInternalTable(T dst, U src) {
    GetInternalTableRef(dst) = GetInternalTable(src);
}
