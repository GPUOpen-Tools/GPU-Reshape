// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
