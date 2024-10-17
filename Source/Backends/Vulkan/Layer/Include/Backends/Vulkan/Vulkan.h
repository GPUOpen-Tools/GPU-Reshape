// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
template<typename T>
inline const T* FindStructureTypeSafe(const void* _struct, VkStructureType structureType) {
    while (_struct) {
        auto* header = static_cast<const StructureType*>(_struct);
        if (header->type == structureType) {
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

/// Prepend an extension structure
inline void PrependExtensionUnsafe(void* _struct, void* extension) {
    auto* structHeader = static_cast<StructureTypeMutable*>(_struct);
    auto* extensionHeader = static_cast<StructureTypeMutable*>(extension);

    extensionHeader->pNext = structHeader->pNext;
    structHeader->pNext = extension;
}

/// Getter stub
template<typename T>
inline T* GetFirstKHR() {
    return nullptr;
}

/// Get the first KHR callable
template<typename TA, typename... TX>
inline auto GetFirstKHR(TA* first, TX*... args) {
    if (first) {
        return first;
    }

    return GetFirstKHR<TA>(args...);
}
