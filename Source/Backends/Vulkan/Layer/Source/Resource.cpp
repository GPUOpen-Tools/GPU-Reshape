#include <Backends/Vulkan/Resource.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/States/ImageState.h>
#include <Backends/Vulkan/States/SamplerState.h>

// Backend
#include <Backend/IL/ResourceTokenType.h>

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create state
    auto state = new(table->allocators) BufferState;
    state->object = *pBuffer;
    state->table = table;

    // Create mapping template
    state->virtualMapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Buffer);
    state->virtualMapping.puid = table->physicalResourceIdentifierMap.AllocatePUID();
    state->virtualMapping.srb = ~0u;

    // Store lookup
    table->states_buffer.Add(*pBuffer, state);

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

    // Store lookup
    table->states_bufferView.Add(*pView, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateImage(device, pCreateInfo, pAllocator, pImage);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create state
    auto state = new(table->allocators) ImageState;
    state->object = *pImage;
    state->table = table;

    // Create mapping template
    state->virtualMappingTemplate.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Texture);
    state->virtualMappingTemplate.puid = table->physicalResourceIdentifierMap.AllocatePUID();
    state->virtualMappingTemplate.srb = ~0u;

    // Store lookup
    table->states_image.Add(*pImage, state);

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

    // Inherit mapping
    state->virtualMapping = state->parent->virtualMappingTemplate;

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
    state->virtualMapping.type = static_cast<uint32_t>(Backend::IL::ResourceTokenType::Sampler);
    state->virtualMapping.puid = table->physicalResourceIdentifierMap.AllocatePUID();
    state->virtualMapping.srb = ~0u;

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
