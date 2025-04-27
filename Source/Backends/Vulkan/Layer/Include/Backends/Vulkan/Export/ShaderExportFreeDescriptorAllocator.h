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

// Std
#include <vector>
#include <mutex>

// Forward declarations
class DeviceAllocator;
struct DeviceDispatchTable;

struct ShaderExportFreeDescriptorAllocation {
    /// Allocated set
    VkDescriptorSet descriptorSet;

    /// Owning pool this set belongs to
    uint32_t poolIndex = UINT32_MAX;
};

struct ShaderExportFreeDescriptorAllocator {
    /// Constructor
    /// @param table parent d evice table
    ShaderExportFreeDescriptorAllocator(DeviceDispatchTable* table);

    /// Destructor
    ~ShaderExportFreeDescriptorAllocator();

    /// Allocate a new descriptor set
    /// @param layout expected layout
    /// @return allocation
    ShaderExportFreeDescriptorAllocation Allocate(VkDescriptorSetLayout layout);

    /// Free a set of allocations
    /// @param allocations all allocations
    /// @param count number of allocations
    void Free(const ShaderExportFreeDescriptorAllocation* allocations, uint32_t count);
    
private:
    struct PoolInfo {
        /// Allocated pool
        VkDescriptorPool pool{VK_NULL_HANDLE};
    };

private:
    /// Parent table
    DeviceDispatchTable* table;

    /// Shared lock
    std::mutex mutex;

    /// All allocated pools
    std::vector<PoolInfo> pools;
};

struct ShaderExportFreeDescriptorAllocatorSegment {
    /// Allocate a new set
    /// @param layout expected layout
    /// @return descriptor set
    VkDescriptorSet Allocate(VkDescriptorSetLayout layout) {
        return allocations.emplace_back(allocator->Allocate(layout)).descriptorSet;
    }

    /// Reset this segment
    void Reset() {
        // Free all allocations
        allocator->Free(allocations.data(), static_cast<uint32_t>(allocations.size()));

        // Cleanup
        allocations.clear();
    }

    /// Owning allocator
    ShaderExportFreeDescriptorAllocator* allocator{nullptr};

    /// All lazily created allocations
    std::vector<ShaderExportFreeDescriptorAllocation> allocations;
};
