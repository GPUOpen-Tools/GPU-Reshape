#pragma once

// Common
#include "Allocator/ContainerAllocator.h"

/// Default alignment
static constexpr uint32_t kDefaultAlign = sizeof(void*);

/// Allocation overload
inline void* operator new (size_t size, const Allocators& allocators) {
    return allocators.alloc(allocators.userData, size, kDefaultAlign, {});
}

/// Allocation overload
inline void* operator new[](size_t size, const Allocators& allocators) {
    return allocators.alloc(allocators.userData, size, kDefaultAlign, {});
}

/// Allocation overload
inline void* operator new (size_t size, const Allocators& allocators, AllocationTag tag) {
    return allocators.alloc(allocators.userData, size, kDefaultAlign, tag);
}

/// Allocation overload
inline void* operator new[](size_t size, const Allocators& allocators, AllocationTag tag) {
    return allocators.alloc(allocators.userData, size, kDefaultAlign, tag);
}

/// Placement delete for exception handling
inline void operator delete (void* data, const Allocators& allocators) {
    return allocators.free(allocators.userData, data, kDefaultAlign);
}

/// Placement delete for exception handling
inline void operator delete[](void* data, const Allocators& allocators) {
    return allocators.free(allocators.userData, data, kDefaultAlign);
}

/// Placement delete for exception handling
inline void operator delete (void* data, const Allocators& allocators, AllocationTag) {
    return allocators.free(allocators.userData, data, kDefaultAlign);
}

/// Placement delete for exception handling
inline void operator delete[](void* data, const Allocators& allocators, AllocationTag) {
    return allocators.free(allocators.userData, data, kDefaultAlign);
}

/// Destruction (can't overload deletion)
template<typename T>
inline void destroy(T* object, const Allocators& allocators) {
    if (!object)
        return;

    object->~T();
    allocators.free(allocators.userData, object, kDefaultAlign);
}