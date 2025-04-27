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

#include <Backends/Vulkan/Export/ShaderExportDeviceAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>

// Std
#include <bit>

ShaderExportDeviceAllocation ShaderExportDeviceAllocator::Allocate(DeviceDispatchTable* table, size_t length) {
    Bucket& bucket = GetReuseBucket(length);

    // Free allocation?
    if (!bucket.entries.empty()) {
        AllocationEntry entry = bucket.entries.back();
        entry.allocation.index = static_cast<uint32_t>(allocations.size());
        bucket.entries.pop_back();

        // Keep track of it for lazy frees
        allocations.push_back(LazyAllocationEntry {
            .allocation = entry.allocation
        });

        return entry.allocation;
    }

    // Mapped description
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    info.size = length;

    // Allow device addresses, if the app uses it
    if (table->next_vkGetBufferDeviceAddress) {
        info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    // Allow shader binding tables, if the app uses it
    // TODO: This is a nonsensical place to do this, really, we need buckets based on general usage
    // Which would also decrease general fragmentation
    if (table->next_vkCreateRayTracingPipelinesKHR) {
        info.usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
    }
    
    // Create allocation
    ShaderExportDeviceAllocation allocation{
        .length = length,
        .index = static_cast<uint32_t>(allocations.size())
    };

    // Attempt to create the host buffer
    if (table->next_vkCreateBuffer(table->object, &info, nullptr, &allocation.buffer) != VK_SUCCESS) {
        return {};
    }

    // Get the requirements
    VkMemoryRequirements requirements;
    table->next_vkGetBufferMemoryRequirements(table->object, allocation.buffer, &requirements);

    // TODO: This is a nonsensical place to do this, really, we need buckets based on general usage
    // Which would also decrease general fragmentation
    if (table->next_vkGetBufferDeviceAddress) {
        requirements.alignment = std::lcm(requirements.alignment, table->physicalDeviceRayTracingPipelineProperties.shaderGroupBaseAlignment);
    }
        
    // Allocate buffer data on device
    allocation.allocation = table->deviceAllocator->Allocate(requirements, AllocationResidency::Device);
        
    // Bind against the allocations
    table->deviceAllocator->BindBuffer(allocation.allocation, allocation.buffer);

    // Keep track of it for lazy frees
    allocations.push_back(LazyAllocationEntry {
        .allocation = allocation
    });

    // OK!
    return allocation;
}

void ShaderExportDeviceAllocator::Free(const ShaderExportDeviceAllocation &allocation) {
    Bucket& bucket = GetFreeBucket(allocation.length);

    // Mark as released
    allocations[allocation.index].released = true;

    bucket.entries.push_back(AllocationEntry {
        .allocation = allocation,
        .age = 0
    });
}

void ShaderExportDeviceAllocator::LazyFree() {
    // Free all unreleased allocations
    for (LazyAllocationEntry &entry: allocations) {
        if (!entry.released) {
            Bucket& bucket = GetFreeBucket(entry.allocation.length);

            bucket.entries.push_back(AllocationEntry {
                .allocation = entry.allocation,
                .age = 0
            });
        }
    }

    // Cleanup
    allocations.clear();
}

void ShaderExportDeviceAllocator::Update(const ComRef<DeviceAllocator>& deviceAllocator) {
    for (Bucket& bucket : buckets) {
        // Free all old allocations
         bucket.entries.erase(std::remove_if( bucket.entries.begin(),  bucket.entries.end(), [&](AllocationEntry& entry) {
             if (entry.age++ < 10) {
                 return false;
             }

             deviceAllocator->Free(entry.allocation.allocation);
             return true;
        }),  bucket.entries.end());
    }
}

void ShaderExportDeviceAllocator::Clear() {
    allocations.clear();
    buckets.clear();
}

ShaderExportDeviceAllocator::Bucket & ShaderExportDeviceAllocator::GetFreeBucket(size_t length) {
    unsigned long level;
    ENSURE(_BitScanReverse(&level, static_cast<uint32_t>(std::bit_floor(length))), "Invalid length");

    if (level >= buckets.size()) {
        buckets.resize(level + 1);
    }

    return buckets[level];
}

ShaderExportDeviceAllocator::Bucket & ShaderExportDeviceAllocator::GetReuseBucket(size_t length) {
    unsigned long level;
    ENSURE(_BitScanReverse(&level, static_cast<uint32_t>(std::bit_ceil(length))), "Invalid length");

    if (level >= buckets.size()) {
        buckets.resize(level + 1);
    }

    return buckets[level];
}
