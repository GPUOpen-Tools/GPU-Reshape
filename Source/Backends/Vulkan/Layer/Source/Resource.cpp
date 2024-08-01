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

#include <Backends/Vulkan/Resource.h>
#include <Backends/Vulkan/Queue.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/States/ImageState.h>
#include <Backends/Vulkan/States/SamplerState.h>
#include <Backends/Vulkan/Controllers/VersioningController.h>
#include <Backends/Vulkan/Resource/ResourceInfo.h>
#include <Backends/Vulkan/Translation.h>
#include <Backends/Vulkan/Memory.h>

// Backend
#include <Backend/IL/ResourceTokenType.h>


VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Redirect all queue families
    auto* queueFamilies = ALLOCA_ARRAY(uint32_t, pCreateInfo->queueFamilyIndexCount);
    for (uint32_t i = 0; i < pCreateInfo->queueFamilyIndexCount; i++) {
        queueFamilies[i] = RedirectQueueFamily(table, pCreateInfo->pQueueFamilyIndices[i]);
    }

    // Copy creation info
    VkBufferCreateInfo createInfo = *pCreateInfo;
    createInfo.pQueueFamilyIndices = queueFamilies;

    // Pass down callchain
    VkResult result = table->next_vkCreateBuffer(device, &createInfo, pAllocator, pBuffer);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create state
    auto state = new(table->allocators) BufferState;
    state->object = *pBuffer;
    state->table = table;
    state->createInfo = *pCreateInfo;

    // Create mapping template
    state->virtualMapping.token.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Buffer);
    state->virtualMapping.token.puid = table->physicalResourceIdentifierMap.AllocatePUID();
    state->virtualMapping.token.formatId = static_cast<uint32_t>(Backend::IL::Format::None);
    state->virtualMapping.token.formatSize = 0;
    state->virtualMapping.token.width = static_cast<uint32_t>(pCreateInfo->size);
    state->virtualMapping.token.DefaultViewToRange();

    // Store lookup
    table->states_buffer.Add(*pBuffer, state);

    // Inform controller
    table->versioningController->CreateOrRecommitBuffer(state);

    // Resource flags
    ResourceCreateFlagSet flags = ResourceCreateFlag::None;

    // Report sparse resources
    if (pCreateInfo->flags & VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT) {
        flags |= ResourceCreateFlag::Tiled;
    }

    // Invoke proxies for all handles
    for (const FeatureHookTable &proxyTable: table->featureHookTables) {
        proxyTable.createResource.TryInvoke(ResourceCreateInfo {
            .resource = GetResourceInfoFor(state->virtualMapping, false),
            .createFlags = flags
        });
    }
    
    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateBufferView(device, pCreateInfo, pAllocator, pView);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create state
    auto state = new(table->allocators) BufferViewState;
    state->object = *pView;
    state->parent = table->states_buffer.Get(pCreateInfo->buffer);

    // Inherit mapping
    state->virtualMapping = state->parent->virtualMapping;
    state->virtualMapping.token.viewFormatId = static_cast<uint32_t>(Translate(pCreateInfo->format));
    state->virtualMapping.token.viewFormatSize = GetFormatByteSize(pCreateInfo->format);

    // Report view widths as that of the view format
    state->virtualMapping.token.viewBaseWidth = static_cast<uint32_t>(pCreateInfo->offset / state->virtualMapping.token.viewFormatSize);

    // Optional view range
    uint32_t byteSizeMin1 = std::max(1u, state->virtualMapping.token.viewFormatSize);
    if (pCreateInfo->range != VK_WHOLE_SIZE) {
        state->virtualMapping.token.viewWidth = static_cast<uint32_t>(pCreateInfo->range / byteSizeMin1);
    } else {
        VkDeviceSize baseWidth = state->virtualMapping.token.width * state->virtualMapping.token.formatSize;
        state->virtualMapping.token.viewWidth = state->virtualMapping.token.viewBaseWidth - static_cast<uint32_t>(baseWidth / byteSizeMin1);
    }

    // Store lookup
    table->states_bufferView.Add(*pView, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Redirect all queue families
    auto* queueFamilies = ALLOCA_ARRAY(uint32_t, pCreateInfo->queueFamilyIndexCount);
    for (uint32_t i = 0; i < pCreateInfo->queueFamilyIndexCount; i++) {
        queueFamilies[i] = RedirectQueueFamily(table, pCreateInfo->pQueueFamilyIndices[i]);
    }

    // Copy creation info
    VkImageCreateInfo createInfo = *pCreateInfo;
    createInfo.pQueueFamilyIndices = queueFamilies;

    // Pass down callchain
    VkResult result = table->next_vkCreateImage(device, &createInfo, pAllocator, pImage);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create state
    auto state = new(table->allocators) ImageState;
    state->object = *pImage;
    state->table = table;
    state->createInfo = *pCreateInfo;

    // Validation
    ASSERT(pCreateInfo->extent.depth == 1u || pCreateInfo->arrayLayers == 1u, "Volumetric and sliced image");

    // Create mapping template
    state->virtualMappingTemplate.token.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture);
    state->virtualMappingTemplate.token.puid = table->physicalResourceIdentifierMap.AllocatePUID();
    state->virtualMappingTemplate.token.formatId = static_cast<uint32_t>(Translate(pCreateInfo->format));
    state->virtualMappingTemplate.token.formatSize = GetFormatByteSize(pCreateInfo->format);
    state->virtualMappingTemplate.token.width = pCreateInfo->extent.width;
    state->virtualMappingTemplate.token.height = pCreateInfo->extent.height;
    state->virtualMappingTemplate.token.depthOrSliceCount = std::max(pCreateInfo->arrayLayers, pCreateInfo->extent.depth);
    state->virtualMappingTemplate.token.mipCount = pCreateInfo->mipLevels;
    state->virtualMappingTemplate.token.DefaultViewToRange();

    // Store lookup
    table->states_image.Add(*pImage, state);

    // Inform controller
    table->versioningController->CreateOrRecommitImage(state);

    // Resource flags
    ResourceCreateFlagSet flags = ResourceCreateFlag::None;

    // Report sparse resources
    if (pCreateInfo->flags & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT) {
        flags |= ResourceCreateFlag::Tiled;
    }

    // Invoke proxies for all handles
    for (const FeatureHookTable &proxyTable: table->featureHookTables) {
        proxyTable.createResource.TryInvoke(ResourceCreateInfo {
            .resource = GetResourceInfoFor(state),
            .createFlags = flags
        });
    }

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateImageView(device, pCreateInfo, pAllocator, pView);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create state
    auto state = new(table->allocators) ImageViewState;
    state->object = *pView;
    state->parent = table->states_image.Get(pCreateInfo->image);

    // Expand the subresource for mappings
    VkImageSubresourceRange expandedRange = ExpandImageSubresourceRange(state->parent, pCreateInfo->subresourceRange);

    // Inherit mapping
    state->virtualMapping = state->parent->virtualMappingTemplate;
    state->virtualMapping.token.viewFormatId = static_cast<uint32_t>(Translate(pCreateInfo->format));
    state->virtualMapping.token.viewFormatSize = GetFormatByteSize(pCreateInfo->format);
    state->virtualMapping.token.viewBaseMip = expandedRange.baseMipLevel;
    state->virtualMapping.token.viewMipCount = expandedRange.levelCount;
    state->virtualMapping.token.viewBaseSlice = expandedRange.baseArrayLayer;
    state->virtualMapping.token.viewSliceCount = expandedRange.layerCount;

    // Store lookup
    table->states_imageView.Add(*pView, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateSampler(device, pCreateInfo, pAllocator, pSampler);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create state
    auto state = new(table->allocators) SamplerState;
    state->object = *pSampler;
    state->table = table;

    // Create mapping template
    state->virtualMapping.token.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Sampler);
    state->virtualMapping.token.puid = table->physicalResourceIdentifierMap.AllocatePUID();

    // Store lookup
    table->states_sampler.Add(*pSampler, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Specification allows null destruction
    if (!buffer) {
        return;
    }

    // Get state
    BufferState* state = table->states_buffer.Get(buffer);
    
    // Invoke proxies for all handles
    for (const FeatureHookTable &proxyTable: table->featureHookTables) {
        proxyTable.destroyResource.TryInvoke(GetResourceInfoFor(state->virtualMapping, false));
    }

    // Free memory
    destroy(state->debugName, table->allocators);
    
    // Inform controller
    table->versioningController->DestroyBuffer(state);

    // Has bound memory?
    if (state->memoryTag.owner) {
        FreeMemoryTag(state->memoryTag);
    }

    // Remove the state
    table->states_buffer.Remove(buffer);

    // Pass down callchain
    table->next_vkDestroyBuffer(device, buffer, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyBufferView(VkDevice device, VkBufferView view, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Specification allows null destruction
    if (!view) {
        return;
    }

    // Remove the state
    table->states_bufferView.Remove(view);

    // Pass down callchain
    table->next_vkDestroyBufferView(device, view, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Specification allows null destruction
    if (!image) {
        return;
    }

    // Get state
    ImageState* state = table->states_image.Get(image);
    
    // Invoke proxies for all handles
    for (const FeatureHookTable &proxyTable: table->featureHookTables) {
        proxyTable.destroyResource.TryInvoke(GetResourceInfoFor(state));
    }

    // Free memory
    destroy(state->debugName, table->allocators);

    // Inform controller
    table->versioningController->DestroyImage(state);

    // Has bound memory?
    if (state->memoryTag.owner) {
        FreeMemoryTag(state->memoryTag);
    }

    // Remove the state
    table->states_image.Remove(image);

    // Pass down callchain
    table->next_vkDestroyImage(device, image, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyImageView(VkDevice device, VkImageView view, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Specification allows null destruction
    if (!view) {
        return;
    }

    // Remove the state
    table->states_imageView.Remove(view);

    // Pass down callchain
    table->next_vkDestroyImageView(device, view, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Specification allows null destruction
    if (!sampler) {
        return;
    }

    // Remove the state
    table->states_sampler.Remove(sampler);

    // Pass down callchain
    table->next_vkDestroySampler(device, sampler, pAllocator);
}
