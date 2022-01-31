#include <Backends/Vulkan/Queue.h>
#include <Backends/Vulkan/Instance.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/States/FenceState.h>
#include <Backends/Vulkan/States/QueueState.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include <Backends/Vulkan/Export/ShaderExportStreamer.h>

void CreateQueueState(DeviceDispatchTable *table, VkQueue queue, uint32_t familyIndex) {
    // Create the state
    auto state = new (table->allocators) QueueState();
    state->table = table;
    state->object = queue;
    state->familyIndex = familyIndex;

    // Pool info
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = familyIndex;

    // Attempt to create the pool
    if (table->next_vkCreateCommandPool(table->object, &poolInfo, nullptr, &state->commandPool) != VK_SUCCESS) {
        return;
    }

    // Allocate the streaming state
    state->exportState = table->exportStreamer->AllocateQueueState(state);

    // OK
    table->states_queue.Add(queue, state);
}

VkCommandBuffer QueueState::PopCommandBuffer() {
    if (!commandBuffers.empty()) {
        VkCommandBuffer cmd = commandBuffers.back();
        commandBuffers.pop_back();
        return cmd;
    }

    // Allocation info
    VkCommandBufferAllocateInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    info.commandPool = commandPool;
    info.commandBufferCount = 1;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    // Attempt to allocate command buffer
    VkCommandBuffer cmd{};
    if (table->next_vkAllocateCommandBuffers(table->object, &info, &cmd) != VK_SUCCESS) {
        return nullptr;
    }

    // Patch the dispatch table
    PatchInternalTable(cmd, table->object);

    // OK
    return cmd;
}

void QueueState::PushCommandBuffer(VkCommandBuffer commandBuffer) {
    commandBuffers.push_back(commandBuffer);
}

QueueState::~QueueState() {
    // Destroy the command buffers
    table->next_vkFreeCommandBuffers(table->object, commandPool, commandBuffers.size(), commandBuffers.data());

    // Destroy the pool
    table->next_vkDestroyCommandPool(table->object, commandPool, nullptr);
}

static FenceState* AcquireOrCreateFence(DeviceDispatchTable* table, QueueState* queue, VkFence userFence) {
    // User provided userFence
    if (userFence) {
        return table->states_fence.Get(userFence);
    }

    // Attempt pooled fence
    if (FenceState* state = queue->pools_fences.TryPop()) {
        // Reset the state of the userFence
        if (table->next_vkResetFences(table->object, 1u, &state->object) != VK_SUCCESS) {
            return nullptr;
        }

        // OK
        return state;
    }

    // None available, create the new state
    auto state = new (table->allocators) FenceState;
    state->table = table;

    // Default create info
    VkFenceCreateInfo createInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

    // Create new userFence
    VkResult result = table->next_vkCreateFence(table->object, &createInfo, nullptr, &state->object);
    if (result != VK_SUCCESS) {
        return nullptr;
    }

    // OK
    return state;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence userFence) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(queue));

    // Get the state
    QueueState* queueState = table->states_queue.Get(queue);

    // Acquire fence
    FenceState* fenceState = AcquireOrCreateFence(table, queueState, userFence);

    // Create streamer allocation
    ShaderExportStreamSegment* segment = table->exportStreamer->AllocateSegment();

    // Unwrapped submits
    auto* vkSubmits = ALLOCA_ARRAY(VkSubmitInfo, submitCount + 1);

    // Number of command buffers
    uint32_t commandBufferCount = 0;
    for (uint32_t i = 0; i < submitCount; i++) {
        commandBufferCount += pSubmits[i].commandBufferCount;
    }

    // Unwrapped command buffers
    auto* vkCommandBuffers = ALLOCA_ARRAY(VkCommandBuffer, commandBufferCount);

    // Unwrap all internal states
    for (uint32_t i = 0; i < submitCount; i++) {
        // Unwrap the command buffers
        for (uint32_t bufferIndex = 0; bufferIndex < pSubmits[i].commandBufferCount; bufferIndex++) {
            auto* unwrapped = reinterpret_cast<CommandBufferObject*>(pSubmits[i].pCommandBuffers[bufferIndex]);

            // Create streamer allocation association
            table->exportStreamer->MapSegment(unwrapped->streamState, segment);

            // Store
            vkCommandBuffers[bufferIndex] = unwrapped->object;
        }

        // Copy non wrapped info
        vkSubmits[i].pNext                = pSubmits[i].pNext;
        vkSubmits[i].sType                = pSubmits[i].sType;
        vkSubmits[i].pSignalSemaphores    = pSubmits[i].pSignalSemaphores;
        vkSubmits[i].signalSemaphoreCount = pSubmits[i].signalSemaphoreCount;
        vkSubmits[i].pWaitSemaphores      = pSubmits[i].pWaitSemaphores;
        vkSubmits[i].waitSemaphoreCount   = pSubmits[i].waitSemaphoreCount;
        vkSubmits[i].pWaitDstStageMask    = pSubmits[i].pWaitDstStageMask;

        // Assign unwrapped states
        vkSubmits[i].commandBufferCount   = pSubmits[i].commandBufferCount;
        vkSubmits[i].pCommandBuffers      = vkCommandBuffers;

        // Advance buffers
        vkCommandBuffers += vkSubmits[i].commandBufferCount;
    }

    // Record the streaming patching
    VkCommandBuffer patchCommandBuffer = table->exportStreamer->RecordPatchCommandBuffer(queueState->exportState, segment);

    // Fill patch submission info
    VkSubmitInfo& patchInfo = (vkSubmits[submitCount] = {VK_STRUCTURE_TYPE_SUBMIT_INFO});
    patchInfo.commandBufferCount = 1;
    patchInfo.pCommandBuffers = &patchCommandBuffer;

    // Pass down callchain
    VkResult result = table->next_vkQueueSubmit(queue, submitCount + 1, vkSubmits, fenceState->object);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Notify streamer of submission, enqueue increments reference count
    table->exportStreamer->Enqueue(queueState->exportState, segment, fenceState);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkQueueWaitIdle(VkQueue queue) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(queue));

    // Get the state
    QueueState* queueState = table->states_queue.Get(queue);

    // Pass down callchain
    VkResult result = table->next_vkQueueWaitIdle(queue);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Inform the streamer of the sync point
    table->exportStreamer->SyncPoint(queueState->exportState);

    // Commit bridge data
    BridgeSyncPoint(table->parent);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkDeviceWaitIdle(VkDevice device) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkDeviceWaitIdle(device);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Inform the streamer of the sync point
    table->exportStreamer->SyncPoint();

    // Commit bridge data
    BridgeSyncPoint(table->parent);

    // OK
    return VK_SUCCESS;
}

VkResult Hook_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(queue));

    // Pass down callchain
    VkResult result = table->next_vkQueuePresentKHR(queue, pPresentInfo);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Commit bridge data
    BridgeSyncPoint(table->parent);

    // OK
    return VK_SUCCESS;
}
