#pragma once

/// Ensure the beta extensions are enabled
#define VK_ENABLE_BETA_EXTENSIONS 1

/// No prototypes
#define VK_NO_PROTOTYPES 1

// General compile time toggles
#include "Config.h"

// Vulkan
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_beta.h>

// Common
#include <Common/Alloca.h>

// Layer
#include "Layer.h"

/// Generic structure type
struct StructureType {
    VkStructureType type;
    const void* pNext;
};

/// Generic mutable structure type
struct StructureTypeMutable {
    VkStructureType type;
    void* pNext;
};

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

/// Find a structure type
template<typename T, VkStructureType STRUCTURE_TYPE>
inline const T* FindStructureTypeSafe(const void* _struct) {
    while (_struct) {
        auto* header = static_cast<const StructureType*>(_struct);
        if (header->type == STRUCTURE_TYPE) {
            return static_cast<const T*>(_struct);
        }

        _struct = header->pNext;
    }

    return nullptr;
}

/// Find a mutable structure type, validity is up to the caller
template<typename T, VkStructureType STRUCTURE_TYPE>
inline T* FindStructureTypeMutableUnsafe(const void* _struct) {
    while (_struct) {
        auto* header = static_cast<const StructureType*>(_struct);
        if (header->type == STRUCTURE_TYPE) {
            return const_cast<T*>(static_cast<const T*>(_struct));
        }

        _struct = header->pNext;
    }

    return nullptr;
}

inline void PrependExtensionUnsafe(void* _struct, void* extension) {
    auto* structHeader = static_cast<StructureTypeMutable*>(_struct);
    auto* extensionHeader = static_cast<StructureTypeMutable*>(extension);

    extensionHeader->pNext = structHeader->pNext;
    structHeader->pNext = extension;
}
