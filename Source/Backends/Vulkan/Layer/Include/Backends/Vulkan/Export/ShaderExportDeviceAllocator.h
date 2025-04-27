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

// Layer
#include <Backends/Vulkan/Allocation/Allocation.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

// Forward declarations
class DeviceAllocator;
struct DeviceDispatchTable;

struct ShaderExportDeviceAllocation {
    /// Underlying allocation
    Allocation allocation{};

    /// Underlying resource
    VkBuffer buffer{VK_NULL_HANDLE};

    /// Total length of this allocation
    uint64_t length{0};

    /// Internal index
    uint32_t index{0};
};

class ShaderExportDeviceAllocator {
public:
    /// Allocate a new resource
    /// \param table the device table
    /// \param length expected length
    /// \return allocation
    ShaderExportDeviceAllocation Allocate(DeviceDispatchTable* table, size_t length);

    /// Free an allocation, may be reused before the next recycle
    /// \param allocation allocation to free
    void Free(const ShaderExportDeviceAllocation& allocation);

    /// Free all "live" allocations back to the pool
    void LazyFree();

    /// Update this allocator, freeing old allocations
    /// \param deviceAllocator owning device allocator
    void Update(const ComRef<DeviceAllocator>& deviceAllocator);

    /// Clear allocations, does not free
    void Clear();

private:
    struct AllocationEntry {
        /// Actual allocation
        ShaderExportDeviceAllocation allocation{};

        /// Number of steps this allocation hasn't been used
        uint32_t age{0};
    };
    
    struct Bucket {
        /// All free allocations
        std::vector<AllocationEntry> entries;
    };

    struct LazyAllocationEntry {
        /// Actual allocation
        ShaderExportDeviceAllocation allocation{};

        /// Already been free'd by the user?
        bool released{false};
    };

    /// Get the bucket for a free
    Bucket& GetFreeBucket(size_t length);
    
    /// Get the bucket for an allocation
    Bucket& GetReuseBucket(size_t length);

    /// All allocations this step
    std::vector<LazyAllocationEntry> allocations;

    /// All allocation buckets
    std::vector<Bucket> buckets;
};
