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

#include "Backends/Vulkan/States/RenderPassState.h"
#include "Backends/Vulkan/States/FrameBufferState.h"
#include "Backends/Vulkan/Tables/DeviceDispatchTable.h"

static void ShallowDeepCopy(RenderPassState* state, const VkRenderPassCreateInfo* pCreateInfo) {
    // Setup shallow copy
    VkRenderPassCreateInfo2 shallow{};
    shallow.sType = pCreateInfo->sType;
    shallow.pNext = pCreateInfo->pNext;
    shallow.flags = pCreateInfo->flags;
    shallow.attachmentCount = pCreateInfo->attachmentCount;

    // Local copy
    auto* shallowAttachments  = ALLOCA_ARRAY(VkAttachmentDescription2, pCreateInfo->attachmentCount);

    // Fill copy
    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++) {
        shallowAttachments[i] = {};
        shallowAttachments[i].flags = pCreateInfo->pAttachments[i].flags;
        shallowAttachments[i].format = pCreateInfo->pAttachments[i].format;
        shallowAttachments[i].samples = pCreateInfo->pAttachments[i].samples;
        shallowAttachments[i].loadOp = pCreateInfo->pAttachments[i].loadOp;
        shallowAttachments[i].storeOp = pCreateInfo->pAttachments[i].storeOp;
        shallowAttachments[i].stencilLoadOp = pCreateInfo->pAttachments[i].stencilLoadOp;
        shallowAttachments[i].stencilStoreOp = pCreateInfo->pAttachments[i].stencilStoreOp;
        shallowAttachments[i].initialLayout = pCreateInfo->pAttachments[i].initialLayout;
        shallowAttachments[i].finalLayout = pCreateInfo->pAttachments[i].finalLayout;
    }

    // Set indirections
    shallow.pAttachments = shallowAttachments;

    // Create deep copy
    state->deepCopy.DeepCopy(state->table->allocators, shallow);
}

template<typename T>
static VkResult CreateReconstructionRenderPass(DeviceDispatchTable *table, const T* pCreateInfo, VkRenderPass* out) {
    T info = *pCreateInfo;

    // Types
    using Attachment = std::remove_const_t<std::remove_pointer_t<decltype(T::pAttachments)>>;

    // Copy all attachments
    Attachment* attachments = ALLOCA_ARRAY(Attachment, info.attachmentCount);
    std::memcpy(attachments, pCreateInfo->pAttachments, sizeof(Attachment) * info.attachmentCount);

    // Patch relevant attributes
    for (uint32_t i = 0; i < info.attachmentCount; i++) {
        Attachment& attachment = attachments[i];

        // Reconstruction passes never clear, just preserve the previous contents
        // It may be discarded in the Store, but that shouldn't matter.
        if (attachment.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        }

        // Just preserve the layouts from start to end
        attachment.initialLayout = attachment.finalLayout;
    }
    
    // Try to create
    if constexpr(std::is_same_v<VkRenderPassCreateInfo2, T>) {
        return table->next_vkCreateRenderPass2(table->object, &info, nullptr, out);
    } else {
        return table->next_vkCreateRenderPass(table->object, &info, nullptr, out);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // The vulkan specification provides no guarantees on allocation lifetimes *beyond* destruction.
    // So, we cannot safely keep the handles around. Use the internal allocators instead.
    pAllocator = nullptr;

    // Pass down callchain
    VkResult result = table->next_vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new state
    auto state = new(table->allocators) RenderPassState;
    state->table = table;
    state->object = *pRenderPass;

    // Create a deep copy from the migrated state
    ShallowDeepCopy(state, pCreateInfo);

    // Try to create reconstruction pass
    if (result = CreateReconstructionRenderPass(table, pCreateInfo, &state->reconstructionObject); result != VK_SUCCESS) {
        return result;
    }

    // External user
    state->AddUser();

    // Store lookup
    table->states_renderPass.Add(*pRenderPass, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // The vulkan specification provides no guarantees on allocation lifetimes *beyond* destruction.
    // So, we cannot safely keep the handles around. Use the internal allocators instead.
    pAllocator = nullptr;

    // Pass down callchain
    VkResult result = table->next_vkCreateRenderPass2(device, pCreateInfo, pAllocator, pRenderPass);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new state
    auto state = new(table->allocators) RenderPassState;
    state->table = table;
    state->object = *pRenderPass;

    // Create deep copy
    state->deepCopy.DeepCopy(table->allocators, *pCreateInfo);

    // Try to create reconstruction pass
    if (result = CreateReconstructionRenderPass(table, pCreateInfo, &state->reconstructionObject); result != VK_SUCCESS) {
        return result;
    }

    // External user
    state->AddUser();

    // Store lookup
    table->states_renderPass.Add(*pRenderPass, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // The vulkan specification provides no guarantees on allocation lifetimes *beyond* destruction.
    // So, we cannot safely keep the handles around. Use the internal allocators instead.
    pAllocator = nullptr;

    // Pass down callchain
    VkResult result = table->next_vkCreateRenderPass2KHR(device, pCreateInfo, pAllocator, pRenderPass);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new state
    auto state = new(table->allocators) RenderPassState;
    state->table = table;
    state->object = *pRenderPass;

    // Create deep copy
    state->deepCopy.DeepCopy(table->allocators, *pCreateInfo);

    // Try to create reconstruction pass
    if (result = CreateReconstructionRenderPass(table, pCreateInfo, &state->reconstructionObject); result != VK_SUCCESS) {
        return result;
    }

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

    // The vulkan specification provides no guarantees on allocation lifetimes *beyond* destruction.
    // So, we cannot safely keep the handles around. Use the internal allocators instead.
    pAllocator = nullptr;

    // Pass down callchain
    VkResult result = table->next_vkCreateFrameBuffer(device, pCreateInfo, pAllocator, pFramebuffer);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new state
    auto state = new(table->allocators) FrameBufferState;
    state->table = table;
    state->object = *pFramebuffer;

    // Validation
    ASSERT(pCreateInfo->pAttachments || (pCreateInfo->flags & VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT), "Expected valid attachments or imageless framebuffer");

    // Get all image views
    if (pCreateInfo->pAttachments) {
        for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++) {
            state->imageViews.push_back(table->states_imageView.Get(pCreateInfo->pAttachments[i]));
        }
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

    // Destroy reconstruction pass
    if (reconstructionObject) {
        table->next_vkDestroyRenderPass(table->object, reconstructionObject, nullptr);
    }

    // Pass down callchain
    table->next_vkDestroyRenderPass(table->object, object, nullptr);
}

inline FrameBufferState::~FrameBufferState() {
    // Remove the state
    table->states_frameBuffers.Remove(object, this);

    // Pass down callchain
    table->next_vkDestroyFrameBuffer(table->object, object, nullptr);
}
