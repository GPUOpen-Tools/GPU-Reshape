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

#include <Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/Export/SegmentInfo.h>
#include <Backends/Vulkan/ShaderData/ShaderDataHost.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTablePersistentVersion.h>

// Common
#include <Common/Containers/TrivialStackVector.h>
#include <Common/Registry.h>

// Backend
#include <Backend/IShaderExportHost.h>

// Std
#include <algorithm>

ShaderExportDescriptorAllocator::ShaderExportDescriptorAllocator(DeviceDispatchTable *table) : table(table) {
}

bool ShaderExportDescriptorAllocator::Install() {
    // Get export host
    auto exportHost = registry->Get<IShaderExportHost>();
    if (!exportHost) {
        return false;
    }

    // Get number of exports
    exportHost->Enumerate(&exportBound, nullptr);

    // Get data host
    auto dataHost = registry->Get<IShaderDataHost>();
    if (!dataHost) {
        return false;
    }

    // Get the number of resources
    uint32_t dataResourceBound;
    dataHost->Enumerate(&dataResourceBound, nullptr, ShaderDataType::DescriptorMask);

    // Get all resources
    dataResources.resize(dataResourceBound);
    dataHost->Enumerate(&dataResourceBound, dataResources.data(), ShaderDataType::DescriptorMask);

    // Descriptors for writing
    TrivialStackVector<VkDescriptorSetLayoutBinding, 16u> bindings(allocators);

    // Binding flags
    //  ? Descriptors are updated latent, during recording we do not know what segment
    //    is appropriate until submission.
    TrivialStackVector<VkDescriptorBindingFlags, 16u> bindingFlags(allocators);

    // Create the binding layout
    CreateBindingLayout();

    // Binding for counter data
    VkDescriptorSetLayoutBinding& counterLayout = bindings.Add({});
    counterLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    counterLayout.stageFlags = VK_SHADER_STAGE_ALL;
    counterLayout.descriptorCount = 1;
    counterLayout.binding = bindingInfo.counterDescriptorOffset;
    bindingFlags.Add(VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);

    // Binding for stream data
    VkDescriptorSetLayoutBinding& streamLayout = bindings.Add({});
    streamLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    streamLayout.stageFlags = VK_SHADER_STAGE_ALL;
    streamLayout.descriptorCount = bindingInfo.streamDescriptorCount;
    streamLayout.binding = bindingInfo.streamDescriptorOffset;
    bindingFlags.Add(VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);

    // Binding for PRMT data
    VkDescriptorSetLayoutBinding& prmtLayout = bindings.Add({});
    prmtLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    prmtLayout.stageFlags = VK_SHADER_STAGE_ALL;
    prmtLayout.descriptorCount = 1;
    prmtLayout.binding = bindingInfo.prmtDescriptorOffset;
    bindingFlags.Add(VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);

    // Binding for descriptor data
    VkDescriptorSetLayoutBinding& descriptorDataLayout = bindings.Add({});
#if PRMT_METHOD == PRMT_METHOD_UB_PC
    descriptorDataLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
#else // PRMT_METHOD == PRMT_METHOD_UB_PC
    descriptorDataLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC
    descriptorDataLayout.stageFlags = VK_SHADER_STAGE_ALL;
    descriptorDataLayout.descriptorCount = 1;
    descriptorDataLayout.binding = bindingInfo.descriptorDataDescriptorOffset;
    bindingFlags.Add(0x0);

    // Binding for constants data
    VkDescriptorSetLayoutBinding& constantsDataLayout = bindings.Add({});
    constantsDataLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    constantsDataLayout.stageFlags = VK_SHADER_STAGE_ALL;
    constantsDataLayout.descriptorCount = 1;
    constantsDataLayout.binding = bindingInfo.shaderDataConstantsDescriptorOffset;
    bindingFlags.Add(0x0);

    // Create data bindings
    for (uint32_t i = 0; i < dataResourceBound; i++) {
        VkDescriptorSetLayoutBinding& dataLayout = bindings.Add({});
        dataLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        dataLayout.stageFlags = VK_SHADER_STAGE_ALL;
        dataLayout.descriptorCount = 1;
        dataLayout.binding = bindingInfo.shaderDataDescriptorOffset + i;
        bindingFlags.Add(0x0);
    }

    // Validate
    ASSERT(bindingFlags.Size() == bindings.Size(), "Mismatched binding to flag count");

    // Binding create info
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    bindingCreateInfo.bindingCount = static_cast<uint32_t>(bindings.Size());
    bindingCreateInfo.pBindingFlags = bindingFlags.Data();

    // Set layout create info
    VkDescriptorSetLayoutCreateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    setInfo.pNext = &bindingCreateInfo;
    setInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    setInfo.bindingCount = static_cast<uint32_t>(bindings.Size());
    setInfo.pBindings = bindings.Data();
    if (table->next_vkCreateDescriptorSetLayout(table->object, &setInfo, nullptr, &layout) != VK_SUCCESS) {
        return false;
    }

    // Create the dummy buffer
    CreateDummyBuffer();

    // OK
    return true;
}

void ShaderExportDescriptorAllocator::CreateBindingLayout() {
    // Current offset
    uint32_t offset{0};

    // Counter info
    bindingInfo.counterDescriptorOffset = offset;
    offset++;

    // Streams
    bindingInfo.streamDescriptorOffset = offset;
    bindingInfo.streamDescriptorCount = exportBound;
    offset += exportBound;

    // PRM table
    bindingInfo.prmtDescriptorOffset = offset;
    offset++;

    // Descriptor data
    bindingInfo.descriptorDataDescriptorOffset = offset;
    bindingInfo.descriptorDataDescriptorLength = std::min<uint32_t>(table->physicalDeviceProperties.limits.maxUniformBufferRange, 256'000);
    offset++;

    // Constants descriptor
    bindingInfo.shaderDataConstantsDescriptorOffset = offset;
    offset++;

    // Data resources
    bindingInfo.shaderDataDescriptorOffset = offset;
    bindingInfo.shaderDataDescriptorCount = static_cast<uint32_t>(dataResources.size());
    offset += static_cast<uint32_t>(dataResources.size());
}

void ShaderExportDescriptorAllocator::CreateDummyBuffer() {
    deviceAllocator = registry->Get<DeviceAllocator>();

    // Dummy buffer info
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.size = sizeof(ShaderExportCounter);

    // Attempt to create the buffer
    if (table->next_vkCreateBuffer(table->object, &bufferInfo, nullptr, &dummyBuffer) != VK_SUCCESS) {
        return;
    }

    // Get the requirements
    VkMemoryRequirements requirements;
    table->next_vkGetBufferMemoryRequirements(table->object, dummyBuffer, &requirements);

    // Create the allocation
    dummyAllocation = deviceAllocator->Allocate(requirements);

    // Bind against the device allocation
    deviceAllocator->BindBuffer(dummyAllocation, dummyBuffer);

    // View creation info
    VkBufferViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    viewInfo.buffer = dummyBuffer;
    viewInfo.format = VK_FORMAT_R32_UINT;
    viewInfo.range = VK_WHOLE_SIZE;

    // Create the view
    if (table->next_vkCreateBufferView(table->object, &viewInfo, nullptr, &dummyBufferView) != VK_SUCCESS) {
        return;
    }
}

ShaderExportDescriptorAllocator::~ShaderExportDescriptorAllocator() {
    // Release all pools
    for (const PoolInfo& info : pools) {
        table->next_vkDestroyDescriptorPool(table->object, info.pool, nullptr);
    }

    // Release the dummy buffer
    table->next_vkDestroyBufferView(table->object, dummyBufferView, nullptr);
    table->next_vkDestroyBuffer(table->object, dummyBuffer, nullptr);

    // Release the dummy allocation
    deviceAllocator->Free(dummyAllocation);

    // Release the set layout
    table->next_vkDestroyDescriptorSetLayout(table->object, layout, nullptr);
}

ShaderExportSegmentDescriptorInfo ShaderExportDescriptorAllocator::Allocate() {
    std::lock_guard guard(mutex);

    // Get pool
    PoolInfo &pool = FindOrAllocatePool();

    // Decrement available sets
    pool.freeSets--;
    ASSERT(pool.freeSets < setsPerPool, "Invalid pool state, max sets per pool exceeded (underflow)");

    // Allocation info
    VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocateInfo.descriptorPool = pool.pool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &layout;

    // Setup the descriptor info
    ShaderExportSegmentDescriptorInfo info;
    info.poolIndex = pool.index;

    // Attempt to allocate a set
    if (table->next_vkAllocateDescriptorSets(table->object, &allocateInfo, &info.set) != VK_SUCCESS) {
        ASSERT(false, "Failed to allocate descriptor sets");
        return {};
    }

    // Single counter
    VkWriteDescriptorSet counterWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    counterWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    counterWrite.descriptorCount = 1u;
    counterWrite.pTexelBufferView = &dummyBufferView;
    counterWrite.dstArrayElement = 0;
    counterWrite.dstSet = info.set;
    counterWrite.dstBinding = bindingInfo.counterDescriptorOffset;

    VkWriteDescriptorSet streamWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    streamWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    streamWrite.descriptorCount = 1u;
    streamWrite.pTexelBufferView = &dummyBufferView;
    streamWrite.dstArrayElement = 0;
    streamWrite.dstSet = info.set;
    streamWrite.dstBinding = bindingInfo.streamDescriptorOffset;

    // Combined writes
    VkWriteDescriptorSet writes[] = {
        counterWrite,
        streamWrite
    };

    // Update the descriptor set
    table->next_vkUpdateDescriptorSets(table->object, 2u, writes, 0, nullptr);

#if LOG_ALLOCATION
    table->parent->logBuffer.Add("Vulkan", LogSeverity::Info, "Allocated segment descriptors");
#endif

    // OK
    return info;
}

ShaderExportDescriptorAllocator::PoolInfo &ShaderExportDescriptorAllocator::FindOrAllocatePool() {
    // Check existing pools
    for (auto it = pools.rbegin(); it != pools.rend(); it++) {
        if (it->freeSets > 0) {
            return *it;
        }
    }

    // Pool size
    VkDescriptorPoolSize poolSizes[] = {
        // Counter + Stream + Data
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .descriptorCount = static_cast<uint32_t>(1u + exportBound + dataResources.size()) * setsPerPool
        },

        // PRMT + Constants
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
            .descriptorCount = 2u * setsPerPool
        },

        // Descriptor
        {
#if PRMT_METHOD == PRMT_METHOD_UB_PC
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
#else // PRMT_METHOD == PRMT_METHOD_UB_PC
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC
            .descriptorCount = 1u * setsPerPool
        },
    };

    // Descriptor pool create info
    VkDescriptorPoolCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.maxSets = setsPerPool;
    createInfo.poolSizeCount = 3u;
    createInfo.pPoolSizes = poolSizes;

    // Create new info
    PoolInfo &info = pools.emplace_back();
    info.index = static_cast<uint32_t>(pools.size() - 1);
    info.freeSets = setsPerPool;

    // Attempt to create the pool
    if (table->next_vkCreateDescriptorPool(table->object, &createInfo, nullptr, &info.pool) != VK_SUCCESS) {
        return info;
    }

    // OK
    return info;
}

void ShaderExportDescriptorAllocator::Free(const ShaderExportSegmentDescriptorInfo &info) {
    std::lock_guard guard(mutex);

    // Get pool
    PoolInfo &pool = pools[info.poolIndex];

    // Increment set count
    pool.freeSets++;
    ASSERT(pool.freeSets <= setsPerPool, "Invalid pool state, max sets per pool exceeded");

    // Free the set
    if (table->next_vkFreeDescriptorSets(table->object, pool.pool, 1, &info.set) != VK_SUCCESS) {
        // ...
    }
}

void ShaderExportDescriptorAllocator::UpdateImmutable(const ShaderExportSegmentDescriptorInfo &info, VkBuffer descriptorChunk, VkBuffer constantsChunk) {
    TrivialStackVector<VkWriteDescriptorSet, 16u> descriptorWrites(allocators);

    // Chunk info
    VkDescriptorBufferInfo chunkBufferInfo;
    chunkBufferInfo.buffer = descriptorChunk ? descriptorChunk : dummyBuffer;
    chunkBufferInfo.offset = 0;
    chunkBufferInfo.range = VK_WHOLE_SIZE;

    // Write descriptor
    VkWriteDescriptorSet& descriptorChunkWrite = descriptorWrites.Add({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
#if PRMT_METHOD == PRMT_METHOD_UB_PC
    descriptorChunkWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
#else // PRMT_METHOD == PRMT_METHOD_UB_PC
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC
    descriptorChunkWrite.descriptorCount = 1u;
    descriptorChunkWrite.pBufferInfo = &chunkBufferInfo;
    descriptorChunkWrite.dstArrayElement = 0;
    descriptorChunkWrite.dstSet = info.set;
    descriptorChunkWrite.dstBinding = bindingInfo.descriptorDataDescriptorOffset;

    // Constants info
    VkDescriptorBufferInfo constantsInfo;
    constantsInfo.buffer = constantsChunk ? constantsChunk : dummyBuffer;
    constantsInfo.offset = 0;
    constantsInfo.range = VK_WHOLE_SIZE;

    // Write descriptor
    VkWriteDescriptorSet& constantChunkWrite = descriptorWrites.Add({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
    constantChunkWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    constantChunkWrite.descriptorCount = 1u;
    constantChunkWrite.pBufferInfo = &constantsInfo;
    constantChunkWrite.dstArrayElement = 0;
    constantChunkWrite.dstSet = info.set;
    constantChunkWrite.dstBinding = bindingInfo.shaderDataConstantsDescriptorOffset;

    // Update the descriptor set
    table->next_vkUpdateDescriptorSets(table->object, static_cast<uint32_t>(descriptorWrites.Size()), descriptorWrites.Data(), 0, nullptr);

    // Create views to shader resources
    table->dataHost->CreateDescriptors(info.set, bindingInfo.shaderDataDescriptorOffset);
}

void ShaderExportDescriptorAllocator::Update(const ShaderExportSegmentDescriptorInfo &info, const ShaderExportSegmentInfo *segment, PhysicalResourceMappingTablePersistentVersion* prmtPersistentVersion) {
    TrivialStackVector<VkWriteDescriptorSet, 16u> descriptorWrites(allocators);

    // PRMT buffer
    VkWriteDescriptorSet& prmtWrite = descriptorWrites.Add({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
    prmtWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    prmtWrite.descriptorCount = 1u;
    prmtWrite.pTexelBufferView = &prmtPersistentVersion->deviceView;
    prmtWrite.dstArrayElement = 0;
    prmtWrite.dstSet = info.set;
    prmtWrite.dstBinding = bindingInfo.prmtDescriptorOffset;
    
    // Single counter
    VkWriteDescriptorSet& counterWrite = descriptorWrites.Add({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
    counterWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    counterWrite.descriptorCount = 1u;
    counterWrite.pTexelBufferView = &segment->counter.view;
    counterWrite.dstArrayElement = 0;
    counterWrite.dstSet = info.set;
    counterWrite.dstBinding = bindingInfo.counterDescriptorOffset;

    // Any streams?
    if (!segment->streams.empty()) {
        auto *streamWrites = ALLOCA_ARRAY(VkBufferView, segment->streams.size());

        // Copy views
        for (size_t i = 0; i < segment->streams.size(); i++) {
            streamWrites[i] = segment->streams[i].view;
        }

        // All streams
        VkWriteDescriptorSet& streamWrite = descriptorWrites.Add({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
        streamWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        streamWrite.descriptorCount = static_cast<uint32_t>(segment->streams.size());
        streamWrite.pTexelBufferView = streamWrites;
        streamWrite.dstArrayElement = 0;
        streamWrite.dstSet = info.set;
        streamWrite.dstBinding = bindingInfo.streamDescriptorOffset;
    }

    // Update the descriptor set
    table->next_vkUpdateDescriptorSets(table->object, static_cast<uint32_t>(descriptorWrites.Size()), descriptorWrites.Data(), 0, nullptr);
}

