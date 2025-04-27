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

#include <Backends/Vulkan/Export/ShaderExportFreeDescriptorAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>

ShaderExportFreeDescriptorAllocator::ShaderExportFreeDescriptorAllocator(DeviceDispatchTable *table) : table(table) {
    
}

ShaderExportFreeDescriptorAllocator::~ShaderExportFreeDescriptorAllocator() {
    // Free all live pools
    for (PoolInfo& pool : pools) {
        table->next_vkDestroyDescriptorPool(table->object, pool.pool, VK_NULL_HANDLE);
    }
}

ShaderExportFreeDescriptorAllocation ShaderExportFreeDescriptorAllocator::Allocate(VkDescriptorSetLayout layout) {
    std::lock_guard lock(mutex);

    ShaderExportFreeDescriptorAllocation allocation;
    
    // Allocation info
    VkDescriptorSetAllocateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    // Search back to front
    for (auto it = pools.rbegin(); it != pools.rend(); it++) {
        info.descriptorPool = it->pool;

        // Try to allocate
        if (table->next_vkAllocateDescriptorSets(table->object, &info, &allocation.descriptorSet) == VK_SUCCESS) {
            allocation.poolIndex = static_cast<uint32_t>(std::distance(it, pools.rbegin()));
            return allocation;
        }
    }

    constexpr uint32_t kMaxSets = 256;

    // Pool sizes
    TrivialStackVector<VkDescriptorPoolSize, 4u> poolSizes;
    poolSizes.Add(VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kMaxSets * 8 });
    poolSizes.Add(VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kMaxSets * 8 });

    // Pool info
    VkDescriptorPoolCreateInfo descriptorPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolInfo.maxSets = kMaxSets;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.Size());
    descriptorPoolInfo.pPoolSizes = poolSizes.Data();

    // Allocate athe pool
    PoolInfo &pool = pools.emplace_back();
    table->next_vkCreateDescriptorPool(table->object, &descriptorPoolInfo, VK_NULL_HANDLE, &pool.pool);

    // Try to allocate against pool
    info.descriptorPool = pool.pool;
    if (table->next_vkAllocateDescriptorSets(table->object, &info, &allocation.descriptorSet) != VK_SUCCESS) {
        return allocation;
    }

    // OK
    allocation.poolIndex = static_cast<uint32_t>(pools.size() - 1);
    return allocation;
}

void ShaderExportFreeDescriptorAllocator::Free(const ShaderExportFreeDescriptorAllocation* allocations, uint32_t count) {
    std::lock_guard lock(mutex);

    // Free all allocations against their pools
    for (uint32_t i = 0; i < count; i++) {
        PoolInfo& pool = pools.at(allocations[i].poolIndex);
        table->next_vkFreeDescriptorSets(table->object, pool.pool, 1u, &allocations[i].descriptorSet);
    }
}
