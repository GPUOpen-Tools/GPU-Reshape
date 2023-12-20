// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#include <Backends/Vulkan/Fence.h>
#include <Backends/Vulkan/States/FenceState.h>
#include "Backends/Vulkan/Tables/DeviceDispatchTable.h"

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // The vulkan specification provides no guarantees on allocation lifetimes *beyond* destruction.
    // So, we cannot safely keep the handles around. Use the internal allocators instead.
    pAllocator = nullptr;

    // Pass down callchain
    VkResult result = table->next_vkCreateFence(device, pCreateInfo, pAllocator, pFence);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Create the new state
    auto state = new (table->allocators) FenceState;
    state->table = table;
    state->object = *pFence;

    // Already signalled?
    if (pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT) {
        state->signallingState = true;
    }

    // External user
    state->AddUser();

    // Store lookup
    table->states_fence.Add(*pFence, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_PTR Hook_vkGetFenceStatus(VkDevice device, VkFence fence) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Get the state
    FenceState* state = table->states_fence.Get(fence);

    // Pass down callchain
    VkResult result = table->next_vkGetFenceStatus(device, fence);

    // If not signalled yet, and fence is done, advance the commit
    if (!state->signallingState && result == VK_SUCCESS) {
        state->signallingState = true;
        state->cpuSignalCommitId++;
    }

    // Up again
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkWaitForFences(device, fenceCount, pFences, waitAll, timeout);

    // Update states of all fences
    for (uint32_t i = 0; i < fenceCount; i++) {
        // Get the state
        FenceState* state = table->states_fence.Get(pFences[i]);

        // Check fence status
        VkResult fenceStatus = table->next_vkGetFenceStatus(device, pFences[i]);

        // If not signalled yet, and fence is done, advance the commit
        if (!state->signallingState && fenceStatus == VK_SUCCESS) {
            state->signallingState = true;
            state->cpuSignalCommitId++;
        }
    }

    // OK
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // All fences
    for (uint32_t i = 0; i < fenceCount; i++) {
        // Get the state
        FenceState* state = table->states_fence.Get(pFences[i]);

        // Reset signalling state
        state->signallingState = false;
    }

    // Pass down callchain
    return table->next_vkResetFences(device, fenceCount, pFences);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyFence(VkDevice device, VkFence Fence, const VkAllocationCallbacks *pAllocator) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Null destruction is allowed by the standard
    if (!Fence) {
        return;
    }

    // Get the state
    FenceState* state = table->states_fence.Get(Fence);

    /* Object deletion deferred to reference destruction */
    destroyRef(state, table->allocators);
}

FenceState::~FenceState() {
    // Remove the state
    table->states_fence.Remove(object, this);

    // Pass down callchain
    table->next_vkDestroyFence(table->object, object, nullptr);
}

uint64_t FenceState::GetLatestCommit() {
    // Pass down callchain
    VkResult result = table->next_vkGetFenceStatus(table->object, object);

    // If not signalled yet, and fence is done, advance the commit
    if (!signallingState && result == VK_SUCCESS) {
        signallingState = true;
        cpuSignalCommitId++;
    }

    // Return new commit
    return cpuSignalCommitId;
}
