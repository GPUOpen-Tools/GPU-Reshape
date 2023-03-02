#pragma once

// Common
#include "AllocatorTag.h"

/// Default allocator
inline void *AllocateDefault(void*, size_t size, size_t align, AllocationTag) {
    return malloc(size);
}

/// Default free
inline void FreeDefault(void*, void *ptr, size_t align) {
    free(ptr);
}

/// Allocator callbacks
using TAllocatorAlloc  = void*(*)(void* user, size_t size, size_t align, AllocationTag tag);
using TAllocatorFree = void(*)(void* user, void* ptr, size_t align);

/// Contains basic allocators
struct Allocators {
    void* userData{nullptr};

    /// Current allocation tag
    AllocationTag tag = kDefaultAllocTag;

    /// Allocate handler
    TAllocatorAlloc alloc = AllocateDefault;

    /// Free handler
    TAllocatorFree free = FreeDefault;

    /// Set the new tag
    /// \param _tag given tag
    /// \return new allocators
    Allocators Tag(const AllocationTag& _tag) const {
        Allocators self = *this;
        self.tag = _tag;
        return self;
    }

    /// Check for equality
    bool operator==(const Allocators& other) const {
        return userData == other.userData && alloc == other.alloc && free == other.free;
    }
};