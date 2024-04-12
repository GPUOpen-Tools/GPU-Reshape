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

// Layer
#include <Backends/Vulkan/Memory.h>
#include <Backends/Vulkan/Resource.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/DeviceMemoryState.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/States/ImageState.h>

// Backend
#include <Backend/Resource/ResourceInfo.h>
#include <Backend/Resource/ResourceToken.h>

VKAPI_ATTR VkResult VKAPI_ATTR Hook_vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create state
    auto state = new(table->allocators) DeviceMemoryState;
    state->object = *pMemory;
    state->table = table;
    state->length = pAllocateInfo->allocationSize;

    // Store lookup
    table->states_deviceMemory.Add(*pMemory, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_ATTR Hook_vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Specification allows null destruction
    if (!memory) {
        return;
    }

    // Remove the state
    table->states_deviceMemory.Remove(memory);

    // Pass down callchain
    table->next_vkFreeMemory(device, memory, pAllocator);
}

static ResourceInfo GetResourceInfoFor(const DeviceMemoryResource& resource) {
    switch (resource.type) {
        default:
            ASSERT(false, "Invalid resource type");
            return {};
        case DeviceMemoryResourceType::Buffer:
            return GetResourceInfoFor(resource.buffer->virtualMapping, false);
        case DeviceMemoryResourceType::Image:
            return GetResourceInfoFor(resource.image);
    }
}

VKAPI_ATTR VkResult VKAPI_ATTR Hook_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkMapMemory(device, memory, offset, size, flags, ppData);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Get states
    DeviceMemoryState* memoryState = table->states_deviceMemory.Get(memory);

    // Work on the actual length by capacity
    if (size == VK_WHOLE_SIZE) {
        size = memoryState->length - offset;
    }

    // Serial!
    std::lock_guard guard(memoryState->lock);
    memoryState->mappedOffset = offset;
    memoryState->mappedLength = size;

    // Find ranges to visit
    auto range = memoryState->range.entries.enumerate(offset, offset + size);
    
    // Invoke proxies for all resources in overlapping range
    for (auto&& [offset, memoryEntry] : range) {
        for (const DeviceMemoryResource& resource : memoryEntry.resources) {
            ResourceInfo info = GetResourceInfoFor(resource);
            
            for (const FeatureHookTable &proxyTable : table->featureHookTables) {
                proxyTable.mapResource.TryInvoke(info);
            }
        }
    }

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_ATTR Hook_vkUnmapMemory(VkDevice device, VkDeviceMemory memory) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    table->next_vkUnmapMemory(device, memory);

    // Get states
    DeviceMemoryState* memoryState = table->states_deviceMemory.Get(memory);

    // Serial!
    std::lock_guard guard(memoryState->lock);

    // Find ranges to visit
    auto range = memoryState->range.entries.enumerate(memoryState->mappedOffset, memoryState->mappedOffset + memoryState->mappedLength);
    
    // Invoke proxies for all resources in overlapping range
    for (auto&& [offset, memoryEntry] : range) {
        for (const DeviceMemoryResource& resource : memoryEntry.resources) {
            ResourceInfo info = GetResourceInfoFor(resource);
            
            for (const FeatureHookTable &proxyTable : table->featureHookTables) {
                proxyTable.unmapResource.TryInvoke(info);
            }
        }
    }
}

static void EmulateBindOverMappedRange(DeviceDispatchTable* table, DeviceMemoryState* memoryState, const BufferState* bufferState, VkDeviceSize memoryOffset) {
    // Not mapped at all?
    if (!memoryState->mappedMemory) {
        return;
    }
    
    // Get length of allocation
    VkMemoryRequirements requirements;
    table->next_vkGetBufferMemoryRequirements(table->object, bufferState->object, &requirements);

    // No overlap?
    if (memoryOffset < memoryState->mappedOffset + memoryState->mappedLength ||
        memoryState->mappedOffset > memoryOffset + requirements.size) {
        return;
    }

    // Get info
    ResourceInfo info = GetResourceInfoFor(bufferState->virtualMapping, false);

    // Inform the features that it is mapped
    for (const FeatureHookTable &proxyTable : table->featureHookTables) {
        proxyTable.mapResource.TryInvoke(info);
    }
}

static void EmulateBindOverMappedRange(DeviceDispatchTable* table, DeviceMemoryState* memoryState, const ImageState* imageState, VkDeviceSize memoryOffset) {
    // Not mapped at all?
    if (!memoryState->mappedMemory) {
        return;
    }
    
    // Get length of allocation
    VkMemoryRequirements requirements;
    table->next_vkGetImageMemoryRequirements(table->object, imageState->object, &requirements);

    // No overlap?
    if (memoryOffset < memoryState->mappedOffset + memoryState->mappedLength ||
        memoryState->mappedOffset > memoryOffset + requirements.size) {
        return;
    }

    // Get info
    ResourceInfo info = GetResourceInfoFor(imageState);

    // Inform the features that it is mapped
    for (const FeatureHookTable &proxyTable : table->featureHookTables) {
        proxyTable.mapResource.TryInvoke(info);
    }
}

VKAPI_ATTR VkResult VKAPI_ATTR Hook_vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkBindBufferMemory(device, buffer, memory, memoryOffset);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Get states
    DeviceMemoryState* memoryState = table->states_deviceMemory.Get(memory);
    BufferState*       bufferState = table->states_buffer.Get(buffer);

    // Serial!
    std::lock_guard guard(memoryState->lock);

    // Get or create resource entry
    DeviceMemoryEntry& entry = memoryState->range.entries[memoryOffset];
    entry.baseOffset = memoryOffset;
    entry.resources.push_back(DeviceMemoryResource::Buffer(bufferState));

    // Handle already mapped ranges
    EmulateBindOverMappedRange(table, memoryState, bufferState, memoryOffset);

    // Set state tag
    ASSERT(!bufferState->memoryTag.owner, "Re-assigned memory tag");
    bufferState->memoryTag.owner = memoryState;
    bufferState->memoryTag.opaque = bufferState;
    bufferState->memoryTag.baseOffset = memoryOffset;

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_ATTR Hook_vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkBindImageMemory(device, image, memory, memoryOffset);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Get states
    DeviceMemoryState* memoryState = table->states_deviceMemory.Get(memory);
    ImageState*        imageState = table->states_image.Get(image);

    // Serial!
    std::lock_guard guard(memoryState->lock);

    // Get or create resource entry
    DeviceMemoryEntry& entry = memoryState->range.entries[memoryOffset];
    entry.baseOffset = memoryOffset;
    entry.resources.push_back(DeviceMemoryResource::Image(imageState));

    // Handle already mapped ranges
    EmulateBindOverMappedRange(table, memoryState, imageState, memoryOffset);

    // Set state tag
    ASSERT(!imageState->memoryTag.owner, "Re-assigned memory tag");
    imageState->memoryTag.owner = memoryState;
    imageState->memoryTag.opaque = imageState;
    imageState->memoryTag.baseOffset = memoryOffset;
    
    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_ATTR Hook_vkBindBufferMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo* pBindInfos) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkBindBufferMemory2KHR(device, bindInfoCount, pBindInfos);
    if (result != VK_SUCCESS) {
        return result;
    }

    for (uint32_t i = 0; i < bindInfoCount; i++) {
        const VkBindBufferMemoryInfo& info = pBindInfos[i];

        // Get states
        DeviceMemoryState* memoryState = table->states_deviceMemory.Get(info.memory);
        BufferState*       bufferState = table->states_buffer.Get(info.buffer);

        // Serial!
        std::lock_guard guard(memoryState->lock);

        // Get or create resource entry
        DeviceMemoryEntry& entry = memoryState->range.entries[info.memoryOffset];
        entry.baseOffset = info.memoryOffset;
        entry.resources.push_back(DeviceMemoryResource::Buffer(bufferState));

        // Handle already mapped ranges
        EmulateBindOverMappedRange(table, memoryState, bufferState, info.memoryOffset);

        // Set state tag
        ASSERT(!bufferState->memoryTag.owner, "Re-assigned memory tag");
        bufferState->memoryTag.owner = memoryState;
        bufferState->memoryTag.opaque = bufferState;
        bufferState->memoryTag.baseOffset = info.memoryOffset;
    }

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_ATTR Hook_vkBindImageMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo* pBindInfos) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkBindImageMemory2KHR(device, bindInfoCount, pBindInfos);
    if (result != VK_SUCCESS) {
        return result;
    }

    for (uint32_t i = 0; i < bindInfoCount; i++) {
        const VkBindImageMemoryInfo& info = pBindInfos[i];

        // Get states
        DeviceMemoryState* memoryState = table->states_deviceMemory.Get(info.memory);
        ImageState*        imageState = table->states_image.Get(info.image);

        // Serial!
        std::lock_guard guard(memoryState->lock);

        // Get or create resource entry
        DeviceMemoryEntry& entry = memoryState->range.entries[info.memoryOffset];
        entry.baseOffset = info.memoryOffset;
        entry.resources.push_back(DeviceMemoryResource::Image(imageState));

        // Handle already mapped ranges
        EmulateBindOverMappedRange(table, memoryState, imageState, info.memoryOffset);

        // Set state tag
        ASSERT(!imageState->memoryTag.owner, "Re-assigned memory tag");
        imageState->memoryTag.owner = memoryState;
        imageState->memoryTag.opaque = imageState;
        imageState->memoryTag.baseOffset = info.memoryOffset;
    }

    // OK
    return VK_SUCCESS;
}

void FreeMemoryTag(const DeviceMemoryTag& tag) {
    std::lock_guard guard(tag.owner->lock);
    
    // Must have owning range
    auto it = tag.owner->range.entries.find(tag.baseOffset);
    if (it == tag.owner->range.entries.end()) {
        ASSERT(false, "Invalid memory tag");
        return;
    }

    bool found = false;

    // Find resource
    for (size_t i = 0; i < it->second.resources.size(); i++) {
        const DeviceMemoryResource& resource = it->second.resources[i];

        // Match?
        if (resource.opaque != tag.opaque) {
            continue;
        }

        // Remove from entry
        it->second.resources.erase(it->second.resources.begin() + i);
        found = true;
        break;
    }

    // Must exist
    ASSERT(found, "Invalid memory tag");
}
