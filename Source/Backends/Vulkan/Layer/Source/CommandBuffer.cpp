#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/States/CommandPoolState.h>
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/States/PipelineLayoutState.h>
#include <Backends/Vulkan/Controllers/InstrumentationController.h>
#include <Backends/Vulkan/Export/ShaderExportStreamer.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>

// Backend
#include <Backends/Vulkan/Command/UserCommandBuffer.h>
#include <Backend/IFeature.h>

// Common
#include "Common/Enum.h"

void CreateDeviceCommandProxies(DeviceDispatchTable *table) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(table->features.size()); i++) {
        const ComRef<IFeature>& feature = table->features[i];

        // Get the hook table
        FeatureHookTable hookTable = feature->GetHookTable();

        /** Create all relevant proxies */

        if (hookTable.drawInstanced.IsValid()) {
            table->commandBufferDispatchTable.featureHooks_vkCmdDraw[i] = hookTable.drawInstanced;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdDraw |= (1ull << i);
        }

        if (hookTable.drawIndexedInstanced.IsValid()) {
            table->commandBufferDispatchTable.featureHooks_vkCmdDrawIndexed[i] = hookTable.drawIndexedInstanced;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdDrawIndexed |= (1ull << i);
        }

        if (hookTable.dispatch.IsValid()) {
            table->commandBufferDispatchTable.featureHooks_vkCmdDispatch[i] = hookTable.dispatch;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdDispatch |= (1ull << i);
        }

        if (hookTable.copyResource.IsValid()) {
            table->commandBufferDispatchTable.featureHooks_vkCmdCopyBuffer[i] = hookTable.copyResource;
            table->commandBufferDispatchTable.featureHooks_vkCmdCopyImage[i] = hookTable.copyResource;
            table->commandBufferDispatchTable.featureHooks_vkCmdCopyImageToBuffer[i] = hookTable.copyResource;
            table->commandBufferDispatchTable.featureHooks_vkCmdCopyBufferToImage[i] = hookTable.copyResource;
            table->commandBufferDispatchTable.featureHooks_vkCmdBlitImage[i] = hookTable.copyResource;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdCopyBuffer |= (1ull << i);
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdCopyImage |= (1ull << i);
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdCopyImageToBuffer |= (1ull << i);
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdCopyBufferToImage |= (1ull << i);
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdBlitImage |= (1ull << i);
        }
        
        if (hookTable.resolveResource.IsValid()) {
            table->commandBufferDispatchTable.featureHooks_vkCmdResolveImage[i] = hookTable.resolveResource;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdResolveImage |= (1ull << i);
        }
        
        if (hookTable.clearResource.IsValid()) {
            table->commandBufferDispatchTable.featureHooks_vkCmdClearAttachments[i] = hookTable.clearResource;
            table->commandBufferDispatchTable.featureHooks_vkCmdClearColorImage[i] = hookTable.clearResource;
            table->commandBufferDispatchTable.featureHooks_vkCmdClearDepthStencilImage[i] = hookTable.clearResource;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdClearAttachments |= (1ull << i);
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdClearColorImage |= (1ull << i);
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdClearDepthStencilImage |= (1ull << i);
        }
    }
}

void SetDeviceCommandFeatureSetAndCommit(DeviceDispatchTable *table, uint64_t featureSet) {
    std::lock_guard lock(table->commandBufferMutex);

    table->commandBufferDispatchTable.featureBitSet_vkCmdDraw = table->commandBufferDispatchTable.featureBitSetMask_vkCmdDraw & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdDrawIndexed = table->commandBufferDispatchTable.featureBitSetMask_vkCmdDrawIndexed & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdDispatch = table->commandBufferDispatchTable.featureBitSetMask_vkCmdDispatch & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdCopyBuffer = table->commandBufferDispatchTable.featureBitSetMask_vkCmdCopyBuffer & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdCopyImage = table->commandBufferDispatchTable.featureBitSetMask_vkCmdCopyImage & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdCopyBufferToImage = table->commandBufferDispatchTable.featureBitSetMask_vkCmdCopyBufferToImage & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdCopyImageToBuffer = table->commandBufferDispatchTable.featureBitSetMask_vkCmdCopyImageToBuffer & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdBlitImage = table->commandBufferDispatchTable.featureBitSetMask_vkCmdBlitImage & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdUpdateBuffer = table->commandBufferDispatchTable.featureBitSetMask_vkCmdUpdateBuffer & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdFillBuffer = table->commandBufferDispatchTable.featureBitSetMask_vkCmdFillBuffer & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdClearColorImage = table->commandBufferDispatchTable.featureBitSetMask_vkCmdClearColorImage & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdClearDepthStencilImage = table->commandBufferDispatchTable.featureBitSetMask_vkCmdClearDepthStencilImage & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdClearAttachments = table->commandBufferDispatchTable.featureBitSetMask_vkCmdClearAttachments & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdResolveImage = table->commandBufferDispatchTable.featureBitSetMask_vkCmdResolveImage & featureSet;
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
    auto *vkCommandBuffers = ALLOCA_ARRAY(VkCommandBuffer, pAllocateInfo->commandBufferCount);

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

        // Allocate the streaming state
        wrapped->streamState = table->exportStreamer->AllocateStreamState();

        // Ensure the internal dispatch table is preserved
        wrapped->next_dispatchTable = GetInternalTable(vkCommandBuffers[i]);
        PatchInternalTable(vkCommandBuffers[i], device);

        // OK
        poolState->commandBuffers.push_back(wrapped);
        pCommandBuffers[i] = wrapped;
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkBeginCommandBuffer(CommandBufferObject *commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    // Pass down the controller
    commandBuffer->table->instrumentationController->BeginCommandList();

    // Acquire the device command table
    {
        std::lock_guard lock(commandBuffer->table->commandBufferMutex);
        commandBuffer->dispatchTable = commandBuffer->table->commandBufferDispatchTable;
    }

    // Pass down callchain
    VkResult result = commandBuffer->table->next_vkBeginCommandBuffer(commandBuffer->object, pBeginInfo);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Begin the streaming state
    commandBuffer->table->exportStreamer->BeginCommandBuffer(commandBuffer->streamState, commandBuffer);

    // Update the PRMT data
    commandBuffer->table->prmTable->Update(commandBuffer->object);

    // Sanity (redundant), reset the context
    commandBuffer->context = {};

    // Cleanup user context
    commandBuffer->userContext.eventStack.Flush();
    commandBuffer->userContext.eventStack.SetRemapping(commandBuffer->table->eventRemappingTable);
    commandBuffer->userContext.buffer.Clear();

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkResetCommandBuffer(CommandBufferObject *commandBuffer, VkCommandBufferResetFlags flags) {
    // Pass down callchain
    VkResult result = commandBuffer->table->next_vkResetCommandBuffer(commandBuffer->object, flags);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Reset export state if present
    if (commandBuffer->streamState) {
        commandBuffer->table->exportStreamer->ResetCommandBuffer(commandBuffer->streamState, commandBuffer);
    }

    // Reset the context
    commandBuffer->context = {};

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdExecuteCommands(CommandBufferObject *commandBuffer, uint32_t commandBufferCount, const CommandBufferObject **pCommandBuffers) {
    auto* unwrapped = ALLOCA_ARRAY(VkCommandBuffer, commandBufferCount);

    // Unwrap
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        unwrapped[i] = pCommandBuffers[i]->object;
    }

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdExecuteCommands(commandBuffer->object, commandBufferCount, unwrapped);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdBindPipeline(CommandBufferObject *commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    // Get state
    PipelineState *state = commandBuffer->table->states_pipeline.Get(pipeline);

    // Attempt to load the hot swapped object
    VkPipeline hotSwapObject = state->hotSwapObject.load();
    if (hotSwapObject) {
        // Replace the bound pipeline by the hot one
        pipeline = hotSwapObject;
    }

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdBindPipeline(commandBuffer->object, pipelineBindPoint, pipeline);

    // Migrate environments
    commandBuffer->table->exportStreamer->BindPipeline(commandBuffer->streamState, state, pipeline, hotSwapObject != nullptr, commandBuffer);

    // Update context
    commandBuffer->context.pipeline = state;
}

static void CommitCompute(CommandBufferObject* commandBuffer) {
    DeviceDispatchTable* table = commandBuffer->table;

    // Commit all commands prior to binding
    CommitCommands(commandBuffer);

    // Inform the streamer
    table->exportStreamer->Commit(commandBuffer->streamState, VK_PIPELINE_BIND_POINT_COMPUTE, commandBuffer);

    // TODO: Update the event data in batches
    if (uint64_t bitMask = commandBuffer->userContext.eventStack.GetGraphicsDirtyMask()) {
        unsigned long index;
        while (_BitScanReverse64(&index, bitMask)) {
            // Push the event data
            commandBuffer->dispatchTable.next_vkCmdPushConstants(
                commandBuffer->object,
                commandBuffer->context.pipeline->layout->object,
                VK_SHADER_STAGE_ALL,
                commandBuffer->context.pipeline->layout->dataPushConstantOffset + index,
                sizeof(uint32_t),
                commandBuffer->userContext.eventStack.GetData() + index
            );

            // Next!
            bitMask &= ~(1ull << index);
        }

        // Cleanup
        commandBuffer->userContext.eventStack.FlushCompute();
    }
}

static void CommitGraphics(CommandBufferObject* commandBuffer) {
    DeviceDispatchTable* table = commandBuffer->table;

    // Commit all commands prior to binding
    CommitCommands(commandBuffer);

    // Inform the streamer
    table->exportStreamer->Commit(commandBuffer->streamState, VK_PIPELINE_BIND_POINT_GRAPHICS, commandBuffer);

    // TODO: Update the event data in batches
    if (uint64_t bitMask = commandBuffer->userContext.eventStack.GetGraphicsDirtyMask()) {
        unsigned long index;
        while (_BitScanReverse64(&index, bitMask)) {
            // Push the event data
            commandBuffer->dispatchTable.next_vkCmdPushConstants(
                commandBuffer->object,
                commandBuffer->context.pipeline->layout->object,
                VK_SHADER_STAGE_ALL,
                commandBuffer->context.pipeline->layout->dataPushConstantOffset + index,
                sizeof(uint32_t),
                commandBuffer->userContext.eventStack.GetData() + index
            );

            // Next!
            bitMask &= ~(1ull << index);
        }

        // Cleanup
        commandBuffer->userContext.eventStack.FlushGraphics();
    }
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdDraw(CommandBufferObject *commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    // Commit all pending graphics
    CommitGraphics(commandBuffer);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdDraw(commandBuffer->object, vertexCount, instanceCount, firstVertex, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdDrawIndexed(CommandBufferObject *commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    // Commit all pending graphics
    CommitGraphics(commandBuffer);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdDrawIndexed(commandBuffer->object, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdDrawIndirect(CommandBufferObject *commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    // Commit all pending graphics
    CommitGraphics(commandBuffer);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdDrawIndirect(commandBuffer->object, buffer, offset, drawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdDrawIndexedIndirect(CommandBufferObject *commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    // Commit all pending graphics
    CommitGraphics(commandBuffer);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdDrawIndexedIndirect(commandBuffer->object, buffer, offset, drawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdDispatch(CommandBufferObject *commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    // Commit all pending compute
    CommitCompute(commandBuffer);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdDispatch(commandBuffer->object, groupCountX, groupCountY, groupCountZ);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdDispatchBase(CommandBufferObject *commandBuffer, uint32_t baseCountX, uint32_t baseCountY, uint32_t baseCountZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    // Commit all pending compute
    CommitCompute(commandBuffer);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdDispatchBase(commandBuffer->object, baseCountX, baseCountY, baseCountZ, groupCountX, groupCountY, groupCountZ);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdDispatchIndirect(CommandBufferObject *commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    // Commit all pending compute
    CommitCompute(commandBuffer);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdDispatchIndirect(commandBuffer->object, buffer, offset);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdDrawIndirectCount(CommandBufferObject *commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    // Commit all pending graphics
    CommitGraphics(commandBuffer);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdDrawIndirectCount(commandBuffer->object, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdDrawIndexedIndirectCount(CommandBufferObject *commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    // Commit all pending graphics
    CommitGraphics(commandBuffer);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdDrawIndexedIndirectCount(commandBuffer->object, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdPushConstants(CommandBufferObject *commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *pValues) {
    // Copy internally
    ASSERT(offset + size <= commandBuffer->streamState->persistentPushConstantData.size(), "Out of bounds push constant range");
    std::memcpy(commandBuffer->streamState->persistentPushConstantData.data() + offset, pValues, size);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdPushConstants(commandBuffer->object, layout, stageFlags, offset, size, pValues);
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkEndCommandBuffer(CommandBufferObject *commandBuffer) {
    // Reset the context
    commandBuffer->context = {};

    // End the streaming state
    commandBuffer->table->exportStreamer->EndCommandBuffer(commandBuffer->streamState, commandBuffer);

    // Pass down callchain
    return commandBuffer->table->next_vkEndCommandBuffer(commandBuffer->object);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, CommandBufferObject **const pCommandBuffers) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Unwrapped states
    auto vkCommandBuffers = ALLOCA_ARRAY(VkCommandBuffer, commandBufferCount);

    // Unwrap and release wrappers
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        vkCommandBuffers[i] = pCommandBuffers[i]->object;

        // Remove from pool
        //  TODO: Slot allocators
        auto poolIt = std::find(pCommandBuffers[i]->pool->commandBuffers.begin(), pCommandBuffers[i]->pool->commandBuffers.end(), pCommandBuffers[i]);
        if (poolIt != pCommandBuffers[i]->pool->commandBuffers.end()) {
            pCommandBuffers[i]->pool->commandBuffers.erase(poolIt);
        }

        // Free the streaming state
        table->exportStreamer->Free(pCommandBuffers[i]->streamState);

        // Free the memory
        destroy(pCommandBuffers[i], table->allocators);
    }

    // Pass down callchain
    table->next_vkFreeCommandBuffers(device, commandPool, commandBufferCount, vkCommandBuffers);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Null destruction is allowed by the standard
    if (!commandPool) {
        return;
    }

    // Get state
    CommandPoolState *state = table->states_commandPool.Get(commandPool);

    // Free all command objects
    for (CommandBufferObject *object: state->commandBuffers) {
        // Free the streaming state
        table->exportStreamer->Free(object->streamState);

        // Destroy the object
        destroy(object, table->allocators);
    }

    // Destroy state
    table->states_commandPool.Remove(commandPool, state);
    destroy(state, table->allocators);

    // Pass down callchain
    table->next_vkDestroyCommandPool(device, commandPool, pAllocator);
}
