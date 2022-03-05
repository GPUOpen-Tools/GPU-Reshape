#pragma once

// Common
#include <Common/Allocators.h>
#include <Common/Assert.h>

// Std
#include <vector>

/// Pool allocation
template<typename T>
struct BucketPoolAllocation {
    /// Array accessor
    T& operator[](uint32_t i) {
        ASSERT(i < count, "Out of bounds read");
        return data[i];
    }

    /// Array accessor
    const T& operator[](uint32_t i) const {
        ASSERT(i < count, "Out of bounds read");
        return data[i];
    }

    /// Check for validity
    operator bool() const {
        return data != nullptr;
    }

    /// All elements, size of [count]
    T* data{nullptr};

    /// Number of elements
    uint32_t count{0};
};

/// Pool allocator
template<typename T>
struct BucketPoolAllocator {
    BucketPoolAllocator(Allocators& allocators) : allocators(allocators) {

    }

    ~BucketPoolAllocator() {
        for (Bucket& bucket : buckets) {
            for (BucketPoolAllocation<T>& allocation : bucket.pool) {
                destroy(allocation.data, allocators);
            }
        }
    }

    /// Create a new allocation
    /// \param count number of elements
    /// \return allocation
    BucketPoolAllocation<T> Allocate(uint32_t count) {
        if (count >= buckets.size()) {
            buckets.resize(count + 1);
        }

        Bucket& bucket = buckets[count];
        if (!bucket.pool.empty()) {
            BucketPoolAllocation<T> allocation = bucket.pool.back();
            bucket.pool.pop_back();
            return allocation;
        }

        BucketPoolAllocation<T> allocation;
        allocation.count = count;
        allocation.data = new (allocators) T[count];
        return allocation;
    }

    /// Free an allocation
    /// \param allocation must be allocated from the same pool
    void Free(const BucketPoolAllocation<T>& allocation) {
        buckets[allocation.count].pool.push_back(allocation);
    }

private:
    struct Bucket {
        std::vector<BucketPoolAllocation<T>> pool;
    };

    /// All buckets
    std::vector<Bucket> buckets;

    /// Shared allocators
    Allocators allocators;
};
