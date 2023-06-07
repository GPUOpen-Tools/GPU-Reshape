#include <Backends/Vulkan/Queue.h>
#include <Backends/Vulkan/Device.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/States/FenceState.h>
#include <Backends/Vulkan/States/QueueState.h>
#include <Backends/Vulkan/States/SwapchainState.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include <Backends/Vulkan/Export/ShaderExportStreamer.h>
#include <Backends/Vulkan/Controllers/VersioningController.h>
#include <Backends/Vulkan/ShaderData/ShaderDataHost.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/OrderedMessageStorage.h>

// Schemas
#include <Schemas/Diagnostic.h>

void CreateQueueState(DeviceDispatchTable *table, VkQueue queue, uint32_t familyIndex) {
    // Create the state
    auto state = new (table->allocators) QueueState();
    state->table = table;
    state->object = queue;
    state->familyIndex = familyIndex;

    // Pool info
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = familyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

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
    if (!commandBuffers.empty()) {
        table->next_vkFreeCommandBuffers(table->object, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    }

    // Destroy the pool
    table->next_vkDestroyCommandPool(table->object, commandPool, nullptr);

    // Release export state
    if (exportState) {
        table->exportStreamer->Free(exportState);
    }
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

        // Next query increments head
        state->signallingState = false;

        // OK
        return state;
    }

    // None available, create the new state
    auto state = new (table->allocators) FenceState;
    state->table = table;
    state->isImmediate = true;

    // Default create info
    VkFenceCreateInfo createInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

    // Create new userFence
    VkResult result = table->next_vkCreateFence(table->object, &createInfo, nullptr, &state->object);
    if (result != VK_SUCCESS) {
        return nullptr;
    }

    // Internal user
    state->AddUser();

    // Store lookup
    table->states_fence.Add(state->object, state);

    // OK
    return state;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence userFence) {
    DeviceDispatchTable* table = DeviceDispatchTable::Get(GetInternalTable(queue));

    // Get the state
    QueueState* queueState = table->states_queue.Get(queue);

    // Check all in-flight streams
    table->exportStreamer->Process(queueState->exportState);

    // Acquire fence
    FenceState* fenceState = AcquireOrCreateFence(table, queueState, userFence);

    // Create streamer allocation
    ShaderExportStreamSegment* segment = table->exportStreamer->AllocateSegment();

    // Inform the controller of the segmentation point
    segment->versionSegPoint = table->versioningController->BranchOnSegmentationPoint();

    // Unwrapped submits
    TrivialStackVector<VkSubmitInfo, 32u> vkSubmits;
    
    // Record the streaming pre patching
    VkCommandBuffer prePatchCommandBuffer = table->exportStreamer->RecordPreCommandBuffer(queueState->exportState, segment);

    // Fill patch submission info
    VkSubmitInfo& prePatchInfo = vkSubmits.Add({VK_STRUCTURE_TYPE_SUBMIT_INFO});
    prePatchInfo.commandBufferCount = 1;
    prePatchInfo.pCommandBuffers = &prePatchCommandBuffer;
    
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

        // Destination
        VkSubmitInfo& submit = vkSubmits.Add();

        // Copy non wrapped info
        submit.pNext                = pSubmits[i].pNext;
        submit.sType                = pSubmits[i].sType;
        submit.pSignalSemaphores    = pSubmits[i].pSignalSemaphores;
        submit.signalSemaphoreCount = pSubmits[i].signalSemaphoreCount;
        submit.pWaitSemaphores      = pSubmits[i].pWaitSemaphores;
        submit.waitSemaphoreCount   = pSubmits[i].waitSemaphoreCount;
        submit.pWaitDstStageMask    = pSubmits[i].pWaitDstStageMask;

        // Assign unwrapped states
        submit.commandBufferCount   = pSubmits[i].commandBufferCount;
        submit.pCommandBuffers      = vkCommandBuffers;

        // Advance buffers
        vkCommandBuffers += submit.commandBufferCount;
    }

    // Record the streaming patching
    VkCommandBuffer postPatchCommandBuffer = table->exportStreamer->RecordPostCommandBuffer(queueState->exportState, segment);

    // Fill patch submission info
    VkSubmitInfo& postPatchInfo = vkSubmits.Add({VK_STRUCTURE_TYPE_SUBMIT_INFO});
    postPatchInfo.commandBufferCount = 1;
    postPatchInfo.pCommandBuffers = &postPatchCommandBuffer;

    // Serialize queue access
    {
        std::lock_guard guard(queueState->mutex);

        // Pass down callchain
        VkResult result = table->next_vkQueueSubmit(queue, static_cast<uint32_t>(vkSubmits.Size()), vkSubmits.Data(), fenceState->object);
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    // Unwrap once again for proxies
    for (uint32_t i = 0; i < submitCount; i++) {
        for (uint32_t bufferIndex = 0; bufferIndex < pSubmits[i].commandBufferCount; bufferIndex++) {
            auto *unwrapped = reinterpret_cast<CommandBufferObject *>(pSubmits[i].pCommandBuffers[bufferIndex]);

            // Invoke all proxies
            for (const FeatureHookTable &proxyTable: table->featureHookTables) {
                proxyTable.submit.TryInvoke(unwrapped->userContext.handle);
            }
        }
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
    table->exportStreamer->Process(queueState->exportState);

    // Commit bridge data
    BridgeDeviceSyncPoint(table);

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
    table->exportStreamer->Process();

    // Commit bridge data
    BridgeDeviceSyncPoint(table);

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

    // Current time
    std::chrono::time_point<std::chrono::high_resolution_clock> presentTime = std::chrono::high_resolution_clock::now();

    // Setup stream
    MessageStream stream;
    MessageStreamView view(stream);

    // Record all elapsed timings
    for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++) {
        SwapchainState* state = table->states_swapchain.Get(pPresentInfo->pSwapchains[i]);

        // Determine elapsed
        float elapsedMS = std::chrono::duration_cast<std::chrono::nanoseconds>(presentTime - state->lastPresentTime).count() / 1e6f;

        // Add message
        auto* diagnostic = view.Add<PresentDiagnosticMessage>();
        diagnostic->intervalMS = elapsedMS;

        // Set new present time
        state->lastPresentTime = presentTime;
    }

    // Commit stream
    table->bridge->GetOutput()->AddStream(stream);

    // Commit bridge data
    BridgeDeviceSyncPoint(table);

    // OK
    return VK_SUCCESS;
}
