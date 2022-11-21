#include <Backends/Vulkan/ShaderData/ShaderDataHost.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Translation.h>

ShaderDataHost::ShaderDataHost(DeviceDispatchTable *table) : table(table) {

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

ShaderDataID ShaderDataHost::CreateBuffer(const ShaderDataBufferInfo &info) {
    // Determine index
    ShaderDataID rid;
    if (freeIndices.empty()) {
        // Allocate at end
        rid = indices.size();
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Set index
    indices[rid] = resources.size();

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.info.id = rid;
    entry.info.type = ShaderDataType::Buffer;
    entry.info.buffer = info;

    // Buffer info
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.size = Backend::IL::GetSize(info.format) * info.elementCount;

    // Attempt to create the buffer
    if (table->next_vkCreateBuffer(table->object, &bufferInfo, nullptr, &entry.buffer) != VK_SUCCESS) {
        return InvalidShaderDataID;
    }

    // Get the requirements
    VkMemoryRequirements requirements;
    table->next_vkGetBufferMemoryRequirements(table->object, entry.buffer, &requirements);

    // Create the allocation
    entry.allocation = deviceAllocator->Allocate(requirements);

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
        rid = indices.size();
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Set index
    indices[rid] = resources.size();

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.allocation = {};
    entry.info.id = rid;
    entry.info.type = ShaderDataType::Event;
    entry.info.event = info;

    // OK
    return rid;
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
