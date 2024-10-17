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

#include "Backend/ShaderData/ShaderDataType.h"
#include <Backends/Vulkan/ShaderData/ShaderDataHost.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Translation.h>
#include <algorithm>
#include <vulkan/vulkan_core.h>

ShaderDataHost::ShaderDataHost(DeviceDispatchTable *table) : table(table) {

}

ShaderDataHost::~ShaderDataHost() {
    for (const ResourceEntry &entry: resources) {
        if (!entry.buffer) {
            continue;
        }

        // Destroy handles
        table->next_vkDestroyBufferView(table->object, entry.view, nullptr);
        table->next_vkDestroyBuffer(table->object, entry.buffer, nullptr);

        // Free associated allocation
        deviceAllocator->Free(entry.allocation);
    }
}

bool ShaderDataHost::Install() {
    // Must have device allocator
    deviceAllocator = registry->Get<DeviceAllocator>();
    if (!deviceAllocator) {
        return false;
    }

    // Fill capability table
    capabilityTable.supportsTiledResources = table->physicalDeviceFeatures.features.sparseResidencyBuffer;
    capabilityTable.bufferMaxElementCount = table->physicalDeviceProperties.limits.maxTexelBufferElements;

    // OK
    return true;
}

void ShaderDataHost::CreateDescriptors(VkDescriptorSet set, uint32_t bindingOffset) {
    std::lock_guard guard(mutex);
    
    TrivialStackVector<VkWriteDescriptorSet, 16u> descriptorWrites(allocators);

    // Add all relevant resources
    for (uint32_t i = 0; i < resources.size(); i++) {
        const ResourceEntry &entry = resources[i];

        // Skip non-descriptor types
        if (!(ShaderDataType::DescriptorMask & entry.info.type)) {
            continue;
        }

        // Handle type
        switch (entry.info.type) {
            default: {
                ASSERT(false, "Invalid resource");
                break;
            }
            case ShaderDataType::Buffer: {
                VkWriteDescriptorSet &descriptorWrite = descriptorWrites.Add({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                descriptorWrite.descriptorCount = 1u;
                descriptorWrite.pTexelBufferView = &entry.view;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.dstSet = set;
                descriptorWrite.dstBinding = bindingOffset + static_cast<uint32_t>(descriptorWrites.Size() - 1u);
                break;
            }
            case ShaderDataType::Texture: {
                ASSERT(false, "Not implemented");
                break;
            }
        }
    }

    // Update the descriptor set
    table->next_vkUpdateDescriptorSets(table->object, static_cast<uint32_t>(descriptorWrites.Size()), descriptorWrites.Data(), 0, nullptr);
}

ShaderDataID ShaderDataHost::CreateBuffer(const ShaderDataBufferInfo &info) {
    std::lock_guard guard(mutex);
    
    // Determine index
    ShaderDataID rid;
    if (freeIndices.empty()) {
        // Allocate at end
        rid = static_cast<uint32_t>(indices.size());
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Set index
    indices[rid] = static_cast<uint32_t>(resources.size());

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.info.id = rid;
    entry.info.type = ShaderDataType::Buffer;
    entry.info.buffer = info;

    // Buffer info
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.size = Backend::IL::GetSize(info.format) * info.elementCount;

    // If tiled, append flags
    if (info.flagSet & ShaderDataBufferFlag::Tiled) {
        bufferInfo.flags |= VK_BUFFER_CREATE_SPARSE_BINDING_BIT | VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
    }

    // Attempt to create the buffer
    if (table->next_vkCreateBuffer(table->object, &bufferInfo, nullptr, &entry.buffer) != VK_SUCCESS) {
        return InvalidShaderDataID;
    }

    // Get the requirements
    table->next_vkGetBufferMemoryRequirements(table->object, entry.buffer, &entry.memoryRequirements);

    // If not tiled, immediately bind memory
    if (!(info.flagSet & ShaderDataBufferFlag::Tiled)) {
        // Create the allocation
        entry.allocation = deviceAllocator->Allocate(entry.memoryRequirements, info.flagSet & ShaderDataBufferFlag::HostVisible ? AllocationResidency::HostVisible : AllocationResidency::Device);

        // Bind against the allocation
        deviceAllocator->BindBuffer(entry.allocation, entry.buffer);
    }

    // View creation info
    VkBufferViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    viewInfo.buffer = entry.buffer;
    viewInfo.format = VK_FORMAT_R32_UINT;
    viewInfo.range = VK_WHOLE_SIZE;

    // Create the view
    if (table->next_vkCreateBufferView(table->object, &viewInfo, nullptr, &entry.view) != VK_SUCCESS) {
        return InvalidShaderDataID;
    }

    // OK
    return rid;
}

ShaderDataID ShaderDataHost::CreateEventData(const ShaderDataEventInfo &info) {
    std::lock_guard guard(mutex);
    
    // Determine index
    ShaderDataID rid;
    if (freeIndices.empty()) {
        // Allocate at end
        rid = static_cast<uint32_t>(indices.size());
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Set index
    indices[rid] = static_cast<uint32_t>(resources.size());

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.allocation = {};
    entry.info.id = rid;
    entry.info.type = ShaderDataType::Event;
    entry.info.event = info;

    // OK
    return rid;
}

ShaderDataID ShaderDataHost::CreateDescriptorData(const ShaderDataDescriptorInfo &info) {
    std::lock_guard guard(mutex);
    
    // Determine index
    ShaderDataID rid;
    if (freeIndices.empty()) {
        // Allocate at end
        rid = static_cast<uint32_t>(indices.size());
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Set index
    indices[rid] = static_cast<uint32_t>(resources.size());

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.allocation = {};
    entry.info.id = rid;
    entry.info.type = ShaderDataType::Descriptor;
    entry.info.descriptor = info;

    // OK
    return rid;
}

void *ShaderDataHost::Map(ShaderDataID rid) {
    std::lock_guard guard(mutex);
    uint32_t index = indices[rid];

    // Entry to map
    ResourceEntry &entry = resources[index];

    // Map it!
    return deviceAllocator->Map(entry.allocation);
}

ShaderDataMappingID ShaderDataHost::CreateMapping(ShaderDataID data, uint64_t tileCount) {
    std::lock_guard guard(mutex);
    
    ResourceEntry &dataEntry = resources[indices[data]];

    // Allocate index
    ShaderDataMappingID mid;
    if (freeMappingIndices.empty()) {
        mid = static_cast<uint32_t>(mappings.size());
        mappings.emplace_back();
    } else {
        mid = freeMappingIndices.back();
        freeMappingIndices.pop_back();
    }

    // Setup requirements, inherit memory bits from the source data
    VkMemoryRequirements requirements;
    requirements.memoryTypeBits = dataEntry.memoryRequirements.memoryTypeBits;
    requirements.alignment = kShaderDataMappingTileWidth;
    requirements.size = tileCount * kShaderDataMappingTileWidth;

    // Create allocation
    MappingEntry& entry = mappings[mid];
    entry.allocation = deviceAllocator->AllocateMemory(requirements);

    // OK
    return mid;
}

void ShaderDataHost::DestroyMapping(ShaderDataMappingID mid) {
    std::lock_guard guard(mutex);
    MappingEntry& entry = mappings[mid];

    // Release the allocation
    deviceAllocator->Free(entry.allocation);

    // Mark as free
    freeMappingIndices.push_back(mid);
}

void ShaderDataHost::FlushMappedRange(ShaderDataID rid, size_t offset, size_t length) {
    std::lock_guard guard(mutex);
    uint32_t index = indices[rid];

    // Entry to flush
    ResourceEntry &entry = resources[index];

    // Flush the range
    deviceAllocator->FlushMappedRange(entry.allocation, offset, length);
}

VkBuffer ShaderDataHost::GetResourceBuffer(ShaderDataID rid) {
    std::lock_guard guard(mutex);
    uint32_t index = indices[rid];

    // Entry to map
    ResourceEntry &entry = resources[index];
    ASSERT(entry.info.type == ShaderDataType::Buffer, "Invalid resource");

    // OK
    return entry.buffer;
}

VmaAllocation ShaderDataHost::GetMappingAllocation(ShaderDataMappingID mid) {
    std::lock_guard guard(mutex);
    MappingEntry &entry = mappings[mid];
    return entry.allocation;
}

void ShaderDataHost::Destroy(ShaderDataID rid) {
    std::lock_guard guard(mutex);
    uint32_t index = indices[rid];

    // Entry to release
    ResourceEntry &entry = resources[index];

    // Release optional resource
    if (entry.buffer) {
        table->next_vkDestroyBufferView(table->object, entry.view, nullptr);
        table->next_vkDestroyBuffer(table->object, entry.buffer, nullptr);
        deviceAllocator->Free(entry.allocation);
    }

    // Not last element?
    if (index != resources.size() - 1) {
        const ResourceEntry &back = resources.back();

        // Swap move last element to current position
        indices[back.info.id] = index;

        // Update indices
        resources[index] = back;
    }

    resources.pop_back();

    // Add as free index
    freeIndices.push_back(rid);
}

void ShaderDataHost::Enumerate(uint32_t *count, ShaderDataInfo *out, ShaderDataTypeSet mask) {
    std::lock_guard guard(mutex);
    
    if (out) {
        uint32_t offset = 0;

        for (uint32_t i = 0; i < resources.size(); i++) {
            if (mask & resources[i].info.type) {
                out[offset++] = resources[i].info;
            }
        }
    } else {
        uint32_t value = 0;

        for (uint32_t i = 0; i < resources.size(); i++) {
            if (mask & resources[i].info.type) {
                value++;
            }
        }

        *count = value;
    }
}

ShaderDataCapabilityTable ShaderDataHost::GetCapabilityTable() {
    return capabilityTable;
}

ConstantShaderDataBuffer ShaderDataHost::CreateConstantDataBuffer() {
    std::lock_guard guard(mutex);
    ConstantShaderDataBuffer out;

    // Total dword count
    uint32_t dwordCount = 0;

    // Summarize size
    for (uint32_t i = 0; i < resources.size(); i++) {
        if (resources[i].info.type == ShaderDataType::Descriptor) {
            dwordCount += resources[i].info.descriptor.dwordCount;
        }
    }

    // Disallow dummy buffer
    if (!dwordCount) {
        return {};
    }

    // Buffer info
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.size = sizeof(uint32_t) * dwordCount;

    // Attempt to create the buffer
    if (table->next_vkCreateBuffer(table->object, &bufferInfo, nullptr, &out.buffer) != VK_SUCCESS) {
        return {};
    }

    // Get the requirements
    VkMemoryRequirements requirements;
    table->next_vkGetBufferMemoryRequirements(table->object, out.buffer, &requirements);

    // Create the allocation
    out.allocation = deviceAllocator->Allocate(requirements);

    // Bind against the allocation
    deviceAllocator->BindBuffer(out.allocation, out.buffer);

    // OK
    return out;
}

ShaderConstantsRemappingTable ShaderDataHost::CreateConstantMappingTable() {
    std::lock_guard guard(mutex);
    
    ShaderConstantsRemappingTable out(indices.size());

    // Current offset
    uint32_t dwordOffset = 0;

    // Accumulate offsets
    for (uint32_t i = 0; i < resources.size(); i++) {
        if (resources[i].info.type == ShaderDataType::Descriptor) {
            out[resources[i].info.id] = dwordOffset;
            dwordOffset += resources[i].info.descriptor.dwordCount;
        }
    }

    // OK
    return out;
}
