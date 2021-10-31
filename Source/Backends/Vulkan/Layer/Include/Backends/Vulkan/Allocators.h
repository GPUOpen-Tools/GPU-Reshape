#pragma once

/// Allocator callbacks
using TAllocatorAlloc  = void*(*)(size_t size);
using TAllocatorFree = void(*)(void* ptr, size_t size);

/// Contains basic allocators
struct Allocators {
    TAllocatorAlloc alloc;
    TAllocatorFree  free;
};

/// Allocation overload
inline void* operator new (size_t size, Allocators& allocators) {
    return allocators.alloc(size);
}

/// Destruction (can't overload deletion)
template<typename T>
inline void destroy(T* object, Allocators& allocators) {
    if (!object)
        return;

    object->~T();
    allocators.free(object, sizeof(T));
}
