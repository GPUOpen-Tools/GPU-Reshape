#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/DeviceDispatchTable.h>
#include <Backends/Vulkan/CommandPoolState.h>
#include <Backends/Vulkan/PipelineState.h>
#include <Backends/Vulkan/Controllers/InstrumentationController.h>

// Backend
#include <Backend/IFeature.h>

void CreateDeviceCommandProxies(DeviceDispatchTable *table) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(table->features.size()); i++) {
        IFeature *feature = table->features[i];

        // Get the hook table
        FeatureHookTable hookTable = feature->GetHookTable();

        // Create all relevant proxies
        if (hookTable.drawIndexed.IsValid()) {
            table->commandBufferDispatchTable.featureHooks_vkCmdDrawIndexed[i] = hookTable.drawIndexed;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdDrawIndexed |= (1ull << i);
        }
    }
}

void SetDeviceCommandFeatureSet(DeviceDispatchTable *table, uint64_t featureSet) {
    table->commandBufferDispatchTable.featureBitSet_vkCmdDrawIndexed = table->commandBufferDispatchTable.featureBitSetMask_vkCmdDrawIndexed & featureSet;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Pass down callchain
    VkResult result = table->next_vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Prepare state
    auto state = new(table->allocators) CommandPoolState();
    table->states_commandPool.Add(*pCommandPool, state);

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, CommandBufferObject **pCommandBuffers) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Get pool state
    CommandPoolState *poolState = table->states_commandPool.Get(pAllocateInfo->commandPool);

    // Returned vulkan handles
    auto *vkCommandBuffers = AllocaArray(VkCommandBuffer, pAllocateInfo->commandBufferCount);

    // Pass down callchain
    VkResult result = table->next_vkAllocateCommandBuffers(device, pAllocateInfo, vkCommandBuffers);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Wrap objects
    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        // Allocate wrapped object
        auto wrapped = new(table->allocators) CommandBufferObject;
        wrapped->object = vkCommandBuffers[i];
        wrapped->table = table;
        wrapped->pool = poolState;

        // Ensure the internal dispatch table is preserved
        wrapped->next_dispatchTable = GetInternalTable(vkCommandBuffers[i]);

        // OK
        poolState->commandBuffers.push_back(wrapped);
        pCommandBuffers[i] = wrapped;
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkBeginCommandBuffer(CommandBufferObject *commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    // Pass down the controller
    commandBuffer->table->instrumentationController->BeginCommandBuffer();

    // Pass down callchain
    return commandBuffer->table->next_vkBeginCommandBuffer(commandBuffer->object, pBeginInfo);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdBindPipeline(CommandBufferObject *commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    // Get state
    PipelineState *state = commandBuffer->table->states_pipeline.Get(pipeline);

    // Attempt to load the hot swapped object
    if (VkPipeline hotSwapObject = state->hotSwapObject.load()) {
        // Replace the bound pipeline by the hot one
        pipeline = hotSwapObject;
    }

    // Pass down callchain
    commandBuffer->table->next_vkCmdBindPipeline(commandBuffer->object, pipelineBindPoint, pipeline);
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkEndCommandBuffer(CommandBufferObject *commandBuffer) {
    // Pass down callchain
    return commandBuffer->table->next_vkEndCommandBuffer(commandBuffer->object);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, CommandBufferObject **const pCommandBuffers) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Unwrapped states
    auto vkCommandBuffers = AllocaArray(VkCommandBuffer, commandBufferCount);

    // Unwrap and release wrappers
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        vkCommandBuffers[i] = pCommandBuffers[i]->object;

        // Remove from pool
        //  TODO: Slot allocators
        auto poolIt = std::find(pCommandBuffers[i]->pool->commandBuffers.begin(), pCommandBuffers[i]->pool->commandBuffers.end(), pCommandBuffers[i]);
        if (poolIt != pCommandBuffers[i]->pool->commandBuffers.end()) {
            pCommandBuffers[i]->pool->commandBuffers.erase(poolIt);
        }

        // Free the memory
        destroy(pCommandBuffers[i], table->allocators);
    }

    // Pass down callchain
    table->next_vkFreeCommandBuffers(device, commandPool, commandBufferCount, vkCommandBuffers);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Get state
    CommandPoolState *state = table->states_commandPool.Get(commandPool);

    // Free all command objects
    for (CommandBufferObject *object: state->commandBuffers) {
        destroy(object, table->allocators);
    }

    // Destroy state
    table->states_commandPool.Remove(commandPool, state);
    destroy(state, table->allocators);

    // Pass down callchain
    table->next_vkDestroyCommandPool(device, commandPool, pAllocator);
}
