#include <Backends/Vulkan/Fence.h>
#include <Backends/Vulkan/States/FenceState.h>
#include "Backends/Vulkan/Tables/DeviceDispatchTable.h"

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

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
