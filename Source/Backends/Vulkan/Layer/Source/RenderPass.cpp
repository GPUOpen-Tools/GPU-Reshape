#include "Backends/Vulkan/States/RenderPassState.h"
#include "Backends/Vulkan/States/FrameBufferState.h"
#include "Backends/Vulkan/Tables/DeviceDispatchTable.h"

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new state
    auto state = new(table->allocators) RenderPassState;
    state->table = table;
    state->object = *pRenderPass;

    // Create deep copy
    state->deepCopy.DeepCopy(table->allocators, *pCreateInfo);

    // External user
    state->AddUser();

    // Store lookup
    table->states_renderPass.Add(*pRenderPass, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateRenderPass2(device, pCreateInfo, pAllocator, pRenderPass);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new state
    auto state = new(table->allocators) RenderPassState;
    state->table = table;
    state->object = *pRenderPass;

    // External user
    state->AddUser();

    // Store lookup
    table->states_renderPass.Add(*pRenderPass, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Null destruction is allowed by the standard
    if (!renderPass) {
        return;
    }

    // Get the state
    RenderPassState *state = table->states_renderPass.Get(renderPass);

    /* Object deletion deferred to reference destruction */
    destroyRef(state, table->allocators);
}


VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateFrameBuffer(device, pCreateInfo, pAllocator, pFramebuffer);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new state
    auto state = new(table->allocators) FrameBufferState;
    state->table = table;
    state->object = *pFramebuffer;

    // Get all image views
    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++) {
        state->imageViews.push_back(table->states_imageView.Get(pCreateInfo->pAttachments[i]));
    }

    // External user
    state->AddUser();

    // Store lookup
    table->states_frameBuffers.Add(*pFramebuffer, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Null destruction is allowed by the standard
    if (!framebuffer) {
        return;
    }

    // Get the state
    FrameBufferState *state = table->states_frameBuffers.Get(framebuffer);

    /* Object deletion deferred to reference destruction */
    destroyRef(state, table->allocators);
}

RenderPassState::~RenderPassState() {
    // Remove the state
    table->states_renderPass.Remove(object, this);

    // Pass down callchain
    table->next_vkDestroyRenderPass(table->object, object, nullptr);
}

inline FrameBufferState::~FrameBufferState() {
    // Remove the state
    table->states_frameBuffers.Remove(object, this);

    // Pass down callchain
    table->next_vkDestroyFrameBuffer(table->object, object, nullptr);
}
