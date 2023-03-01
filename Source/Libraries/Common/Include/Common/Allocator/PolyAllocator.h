#pragma once

// Common
#include <Common/Allocators.h>

// Std
#include <memory_resource>

class PolyAllocator final : public std::pmr::memory_resource {
public:
    PolyAllocator(const Allocators& allocators = {}, const AllocationTag& tag = {}) : allocators(allocators), tag(tag) {
        
    }

    /// Reassign allocators
    void Assign(const Allocators& _allocators = {}, const AllocationTag& _tag = {}) {
        allocators = _allocators;
        tag = _tag;
    }

    /// Allocation
    void* do_allocate(size_t _Bytes, size_t _Align) {
        return allocators.alloc(allocators.userData, _Bytes, _Align, tag);
    }

    /// Deallocation
    void do_deallocate(void *_Ptr, size_t _Bytes, size_t _Align) {
        allocators.free(allocators.userData, _Ptr, _Align);
    }

    /// Equality check
    bool do_is_equal(const memory_resource &_That) const noexcept {
        return allocators == static_cast<const PolyAllocator&>(_That).allocators;
    }

    /// Get the allocators
    const Allocators& GetAllocators() const {
        return allocators;
    }

    /// Get the tag
    const AllocationTag& GetTag() const {
        return tag;
    }

private:
    Allocators    allocators;
    AllocationTag tag;
};
