#pragma once

// Common
#include "CRC.h"

// Std
#include <string_view>
#include <malloc.h>

/// Identifiable allocation tag
struct AllocationTag {
    uint64_t         crc64;
    std::string_view view;
};

/// Tag literal
#if __cplusplus >= 202002L
GRS_CONSTEVAL AllocationTag operator "" _AllocTag(const char* data, size_t len) {
    return AllocationTag {
        .crc64 = crcdetail::compute(data, len),
        .view = std::string_view(data, len)
    };
}
#endif // __cplusplus >= 202002L

/// Default alignment
static constexpr uint32_t kDefaultAlign = sizeof(void*);

/// Allocator callbacks
using TAllocatorAlloc  = void*(*)(void* user, size_t size, size_t align, AllocationTag tag);
using TAllocatorFree = void(*)(void* user, void* ptr, size_t align);

/// Default allocator
inline void *AllocateDefault(void*, size_t size, size_t align, AllocationTag) {
    return malloc(size);
}

/// Default free
inline void FreeDefault(void*, void *ptr, size_t align) {
    free(ptr);
}

/// Contains basic allocators
struct Allocators {
    void* userData{nullptr};
    TAllocatorAlloc alloc = AllocateDefault;
    TAllocatorFree  free = FreeDefault;

    /// Check for equality
    bool operator==(const Allocators& other) const {
        return userData == other.userData && alloc == other.alloc && free == other.free;
    }
};

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