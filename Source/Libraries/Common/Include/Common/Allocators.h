#pragma once

// Std
#include <malloc.h>

/// Allocator callbacks
using TAllocatorAlloc  = void*(*)(size_t size);
using TAllocatorFree = void(*)(void* ptr);

/// Default allocator
inline void *AllocateDefault(size_t size) {
    return malloc(size);
}

/// Default free
inline void FreeDefault(void *ptr) {
    free(ptr);
}

/// Contains basic allocators
struct Allocators {
    TAllocatorAlloc alloc = AllocateDefault;
    TAllocatorFree  free = FreeDefault;
};

/// Allocation overload
inline void* operator new (size_t size, const Allocators& allocators) {
    return allocators.alloc(size);
}

/// Allocation overload
inline void* operator new[](size_t size, const Allocators& allocators) {
    return allocators.alloc(size);
}

/// Destruction (can't overload deletion)
template<typename T>
inline void destroy(T* object, const Allocators& allocators) {
    if (!object)
        return;

    object->~T();
    allocators.free(object);
}