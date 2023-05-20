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

    // OK
    return true;
}

void ShaderDataHost::CreateDescriptors(VkDescriptorSet set, uint32_t bindingOffset) {
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

    // Attempt to create the buffer
    if (table->next_vkCreateBuffer(table->object, &bufferInfo, nullptr, &entry.buffer) != VK_SUCCESS) {
        return InvalidShaderDataID;
    }

    // Get the requirements
    VkMemoryRequirements requirements;
    table->next_vkGetBufferMemoryRequirements(table->object, entry.buffer, &requirements);

    // Create the allocation
    entry.allocation = deviceAllocator->Allocate(requirements, info.hostVisible ? AllocationResidency::HostVisible : AllocationResidency::Device);

    // Bind against the allocation
    deviceAllocator->BindBuffer(entry.allocation, entry.buffer);

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
    uint32_t index = indices[rid];

    // Entry to map
    ResourceEntry &entry = resources[index];

    // Map it!
    return deviceAllocator->Map(entry.allocation);
}

void ShaderDataHost::FlushMappedRange(ShaderDataID rid, size_t offset, size_t length) {
    uint32_t index = indices[rid];

    // Entry to flush
    ResourceEntry &entry = resources[index];

    // Flush the range
    deviceAllocator->FlushMappedRange(entry.allocation, offset, length);
}

VkBuffer ShaderDataHost::GetResourceBuffer(ShaderDataID rid) {
    uint32_t index = indices[rid];

    // Entry to map
    ResourceEntry &entry = resources[index];
    ASSERT(entry.info.type == ShaderDataType::Buffer, "Invalid resource");

    // OK
    return entry.buffer;
}

void ShaderDataHost::Destroy(ShaderDataID rid) {
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

ConstantShaderDataBuffer ShaderDataHost::CreateConstantDataBuffer() {
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
