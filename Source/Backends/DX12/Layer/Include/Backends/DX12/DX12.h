#pragma once

// General compile time toggles
#include "Config.h"

// Common
#include <Common/Alloca.h>
#include <Common/Assert.h>

// System
#include <d3d12.h>
#include <dxgi.h>

// Cleanup
#undef OPAQUE

/// Get the vtable
template<typename T = void>
inline T* GetVTableRaw(void* object) {
    if (!object) {
        return nullptr;
    }

    return *(T **) object;
}

/// Get the vtable
template<typename T = void>
inline T *&GetVTableRawRef(void* object) {
    return *(T **) object;
}