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

#include <Backends/Vulkan/CommandBuffer.h>
#include <Backends/Vulkan/Queue.h>
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

        // Append
        table->featureHookTables.push_back(hookTable);

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
        
        if (hookTable.beginRenderPass.IsValid()) {
            table->commandBufferDispatchTable.featureHooks_vkCmdBeginRenderPass[i] = hookTable.beginRenderPass;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdBeginRenderPass |= (1ull << i);
            table->commandBufferDispatchTable.featureHooks_vkCmdBeginRendering[i] = hookTable.beginRenderPass;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdBeginRendering |= (1ull << i);
            table->commandBufferDispatchTable.featureHooks_vkCmdBeginRenderingKHR[i] = hookTable.beginRenderPass;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdBeginRenderingKHR |= (1ull << i);
        }
        
        if (hookTable.endRenderPass.IsValid()) {
            table->commandBufferDispatchTable.featureHooks_vkCmdEndRenderPass[i] = hookTable.endRenderPass;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdEndRenderPass |= (1ull << i);
            table->commandBufferDispatchTable.featureHooks_vkCmdEndRendering[i] = hookTable.endRenderPass;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdEndRendering |= (1ull << i);
            table->commandBufferDispatchTable.featureHooks_vkCmdEndRenderingKHR[i] = hookTable.endRenderPass;
            table->commandBufferDispatchTable.featureBitSetMask_vkCmdEndRenderingKHR |= (1ull << i);
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
    table->commandBufferDispatchTable.featureBitSet_vkCmdBeginRenderPass = table->commandBufferDispatchTable.featureBitSetMask_vkCmdBeginRenderPass & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdEndRenderPass = table->commandBufferDispatchTable.featureBitSetMask_vkCmdEndRenderPass & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdBeginRendering = table->commandBufferDispatchTable.featureBitSetMask_vkCmdBeginRendering & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdBeginRenderingKHR = table->commandBufferDispatchTable.featureBitSetMask_vkCmdBeginRenderingKHR & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdEndRendering = table->commandBufferDispatchTable.featureBitSetMask_vkCmdEndRendering & featureSet;
    table->commandBufferDispatchTable.featureBitSet_vkCmdEndRenderingKHR = table->commandBufferDispatchTable.featureBitSetMask_vkCmdEndRenderingKHR & featureSet;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Copy creation info
    VkCommandPoolCreateInfo createInfo = *pCreateInfo;
    createInfo.queueFamilyIndex = RedirectQueueFamily(table, createInfo.queueFamilyIndex);

    // Pass down callchain
    VkResult result = table->next_vkCreateCommandPool(device, &createInfo, pAllocator, pCommandPool);
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
    commandBuffer->table->instrumentationController->ConditionalWaitForCompletion();

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
    commandBuffer->table->exportStreamer->BeginCommandBuffer(commandBuffer->streamState, commandBuffer->object);

    // Sanity (redundant), reset the context
    commandBuffer->context = {};

    // Cleanup user context
    commandBuffer->userContext.eventStack.Flush();
    commandBuffer->userContext.eventStack.SetRemapping(commandBuffer->table->eventRemappingTable);
    commandBuffer->userContext.buffer.Clear();
    commandBuffer->userContext.handle = reinterpret_cast<CommandContextHandle>(commandBuffer);

    // Set stream context handle
    commandBuffer->streamState->commandContextHandle = commandBuffer->userContext.handle;

    // Invoke proxies
    for (const FeatureHookTable &table: commandBuffer->table->featureHookTables) {
        table.open.TryInvoke(&commandBuffer->userContext);
    }

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
        commandBuffer->table->exportStreamer->ResetCommandBuffer(commandBuffer->streamState);
    }

    // Reset the context
    commandBuffer->context = {};

    // OK
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkResetCommandPool(VkDevice device, VkCommandPool pool, VkCommandPoolResetFlags flags) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Get pool state
    CommandPoolState *poolState = table->states_commandPool.Get(pool);

    // Pass down callchain
    VkResult result = table->next_vkResetCommandPool(device, pool, flags);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Reset all internal command states
    for (CommandBufferObject* commandBuffer : poolState->commandBuffers) {
        // Reset export state if present
        if (commandBuffer->streamState) {
            commandBuffer->table->exportStreamer->ResetCommandBuffer(commandBuffer->streamState);
        }

        // Reset the context
        commandBuffer->context = {};
    }

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

    // Conditionally wait for instrumentation if the pipeline has an outstanding request
    if (!hotSwapObject && state->HasInstrumentationRequest()) {
        state->table->instrumentationController->ConditionalWaitForCompletion();

        // Load new hot-object
        hotSwapObject = state->hotSwapObject.load();
    }
    
    // Replace the bound pipeline by the hot one
    if (hotSwapObject) {
        pipeline = hotSwapObject;
    }

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdBindPipeline(commandBuffer->object, pipelineBindPoint, pipeline);

    // Migrate environments
    commandBuffer->table->exportStreamer->BindPipeline(commandBuffer->streamState, state, pipeline, hotSwapObject != nullptr, commandBuffer->object);

    // Update context
    commandBuffer->context.pipeline = state;
}

static void CommitCompute(CommandBufferObject* commandBuffer) {
    DeviceDispatchTable* table = commandBuffer->table;

    // Commit all commands prior to binding
    CommitCommands(commandBuffer);

    // Inform the streamer
    table->exportStreamer->Commit(commandBuffer->streamState, VK_PIPELINE_BIND_POINT_COMPUTE, commandBuffer->object);

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
    table->exportStreamer->Commit(commandBuffer->streamState, VK_PIPELINE_BIND_POINT_GRAPHICS, commandBuffer->object);

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

#if PIPELINE_MERGE_PC_RANGES
    // Vulkan requires that the overlapping range flags are the exact same
    // As the merged ranges put everything in a single range, just assume its stage flags.
    stageFlags = VK_SHADER_STAGE_ALL;
#endif // PIPELINE_MERGE_PC_RANGES

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdPushConstants(commandBuffer->object, layout, stageFlags, offset, size, pValues);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdBeginRenderPass(CommandBufferObject* commandBuffer, const VkRenderPassBeginInfo* info, VkSubpassContents contents) {
    // Copy all render pass info
    commandBuffer->streamState->renderPass.subpassContents = contents;
    commandBuffer->streamState->renderPass.deepCopy.DeepCopy(commandBuffer->table->allocators, *info);

    // Mark as inside
    commandBuffer->streamState->renderPass.insideRenderPass = true;

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdBeginRenderPass(commandBuffer->object, info, contents);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdEndRenderPass(CommandBufferObject* commandBuffer) {
    // Mark as outside
    commandBuffer->streamState->renderPass.insideRenderPass = false;
    
    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdEndRenderPass(commandBuffer->object);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdPushDescriptorSetKHR(CommandBufferObject* commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites) {
    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdPushDescriptorSetKHR(commandBuffer->object, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);

    // Inform streamer
    commandBuffer->table->exportStreamer->PushDescriptorSetKHR(commandBuffer->streamState, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites, commandBuffer->object);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdPushDescriptorSetWithTemplateKHR(CommandBufferObject* commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void* pData) {
    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdPushDescriptorSetWithTemplateKHR(commandBuffer->object, descriptorUpdateTemplate, layout, set, pData);

    // Inform streamer
    commandBuffer->table->exportStreamer->PushDescriptorSetWithTemplateKHR(commandBuffer->streamState, descriptorUpdateTemplate, layout, set, pData, commandBuffer->object);
}

template<typename T>
void CopyAndMigrateMemoryBarrier(DeviceDispatchTable *table, T* dest, const T* source, uint32_t count) {
    // Copy data
    std::memcpy(dest, source, sizeof(T) * count);

    // Migrate all families
    for (uint32_t i = 0; i < count; i++) {
        dest[i].dstQueueFamilyIndex = RedirectQueueFamily(table, dest[i].dstQueueFamilyIndex);
        dest[i].srcQueueFamilyIndex = RedirectQueueFamily(table, dest[i].srcQueueFamilyIndex);
    }
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdWaitEvents(CommandBufferObject* commandBuffer, uint32_t eventCount, const VkEvent *pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    // Migrate images
    auto* imageMemoryBarriers  = ALLOCA_ARRAY(VkImageMemoryBarrier, imageMemoryBarrierCount);
    CopyAndMigrateMemoryBarrier(commandBuffer->table, imageMemoryBarriers, pImageMemoryBarriers, imageMemoryBarrierCount);

    // Migrate buffers
    auto* bufferMemoryBarriers = ALLOCA_ARRAY(VkBufferMemoryBarrier, bufferMemoryBarrierCount);
    CopyAndMigrateMemoryBarrier(commandBuffer->table, bufferMemoryBarriers, pBufferMemoryBarriers, bufferMemoryBarrierCount);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdWaitEvents(commandBuffer->object, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, bufferMemoryBarriers, imageMemoryBarrierCount, imageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdPipelineBarrier(CommandBufferObject* commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    // Migrate images
    auto* imageMemoryBarriers  = ALLOCA_ARRAY(VkImageMemoryBarrier, imageMemoryBarrierCount);
    CopyAndMigrateMemoryBarrier(commandBuffer->table, imageMemoryBarriers, pImageMemoryBarriers, imageMemoryBarrierCount);

    // Migrate buffers
    auto* bufferMemoryBarriers = ALLOCA_ARRAY(VkBufferMemoryBarrier, bufferMemoryBarrierCount);
    CopyAndMigrateMemoryBarrier(commandBuffer->table, bufferMemoryBarriers, pBufferMemoryBarriers, bufferMemoryBarrierCount);

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdPipelineBarrier(commandBuffer->object, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, bufferMemoryBarriers, imageMemoryBarrierCount, imageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdSetEvent2(CommandBufferObject* commandBuffer, VkEvent event, const VkDependencyInfo *pDependencyInfo) {
    VkDependencyInfo dependencyInfo = *pDependencyInfo;

    // Migrate images
    auto* imageMemoryBarriers  = ALLOCA_ARRAY(VkImageMemoryBarrier2, dependencyInfo.imageMemoryBarrierCount);
    CopyAndMigrateMemoryBarrier(commandBuffer->table, imageMemoryBarriers, dependencyInfo.pImageMemoryBarriers, dependencyInfo.imageMemoryBarrierCount);

    // Migrate buffers
    auto* bufferMemoryBarriers  = ALLOCA_ARRAY(VkBufferMemoryBarrier2, dependencyInfo.bufferMemoryBarrierCount);
    CopyAndMigrateMemoryBarrier(commandBuffer->table, bufferMemoryBarriers, dependencyInfo.pBufferMemoryBarriers, dependencyInfo.bufferMemoryBarrierCount);

    // Set new barriers
    dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers;
    dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers;

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdSetEvent2(commandBuffer->object, event, &dependencyInfo);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdWaitEvents2(CommandBufferObject* commandBuffer, uint32_t eventCount, const VkEvent *pEvents, const VkDependencyInfo *pDependencyInfos) {
    // Final counts
    uint32_t bufferBarrierCount{0};
    uint32_t imageBarrierCount{0};

    // Accumulate counts
    for (uint32_t i = 0; i < eventCount; i++) {
        bufferBarrierCount += pDependencyInfos[i].bufferMemoryBarrierCount;
        imageBarrierCount += pDependencyInfos[i].imageMemoryBarrierCount;
    }

    // Local barriers
    auto* imageMemoryBarriers  = ALLOCA_ARRAY(VkImageMemoryBarrier2, bufferBarrierCount);
    auto* bufferMemoryBarriers = ALLOCA_ARRAY(VkBufferMemoryBarrier2, imageBarrierCount);

    // Copy dependencies
    auto* dependencies = ALLOCA_ARRAY(VkDependencyInfo, eventCount);
    std::memcpy(dependencies, pDependencyInfos, sizeof(VkDependencyInfo) * eventCount);

    // Rewrite all dependencies
    for (uint32_t i = 0; i < eventCount; i++) {
        // Copy barriers
        CopyAndMigrateMemoryBarrier(commandBuffer->table, imageMemoryBarriers, dependencies[i].pImageMemoryBarriers, dependencies[i].imageMemoryBarrierCount);
        CopyAndMigrateMemoryBarrier(commandBuffer->table, bufferMemoryBarriers, dependencies[i].pBufferMemoryBarriers, dependencies[i].bufferMemoryBarrierCount);

        // Set new barriers
        dependencies[i].pImageMemoryBarriers = imageMemoryBarriers;
        dependencies[i].pBufferMemoryBarriers = bufferMemoryBarriers;

        // Offset barriers
        imageMemoryBarriers += dependencies[i].imageMemoryBarrierCount;
        bufferMemoryBarriers += dependencies[i].bufferMemoryBarrierCount;
    }

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdWaitEvents2(commandBuffer->object, eventCount, pEvents, dependencies);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkCmdPipelineBarrier2(CommandBufferObject* commandBuffer, const VkDependencyInfo *pDependencyInfo) {
    VkDependencyInfo dependencyInfo = *pDependencyInfo;

    // Migrate images
    auto* imageMemoryBarriers  = ALLOCA_ARRAY(VkImageMemoryBarrier2, dependencyInfo.imageMemoryBarrierCount);
    CopyAndMigrateMemoryBarrier(commandBuffer->table, imageMemoryBarriers, dependencyInfo.pImageMemoryBarriers, dependencyInfo.imageMemoryBarrierCount);

    // Migrate buffers
    auto* bufferMemoryBarriers  = ALLOCA_ARRAY(VkBufferMemoryBarrier2, dependencyInfo.bufferMemoryBarrierCount);
    CopyAndMigrateMemoryBarrier(commandBuffer->table, bufferMemoryBarriers, dependencyInfo.pBufferMemoryBarriers, dependencyInfo.bufferMemoryBarrierCount);

    // Set new barriers
    dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers;
    dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers;

    // Pass down callchain
    commandBuffer->dispatchTable.next_vkCmdPipelineBarrier2(commandBuffer->object, &dependencyInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL Hook_vkEndCommandBuffer(CommandBufferObject *commandBuffer) {
    // Reset the context
    commandBuffer->context = {};

    // End the streaming state
    commandBuffer->table->exportStreamer->EndCommandBuffer(commandBuffer->streamState, commandBuffer->object);

    // Invoke proxies
    for (const FeatureHookTable &table: commandBuffer->table->featureHookTables) {
        table.close.TryInvoke(commandBuffer->userContext.handle);
    }

    // Pass down callchain
    return commandBuffer->table->next_vkEndCommandBuffer(commandBuffer->object);
}

VKAPI_ATTR void VKAPI_CALL Hook_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, CommandBufferObject **const pCommandBuffers) {
    DeviceDispatchTable *table = DeviceDispatchTable::Get(GetInternalTable(device));

    // Unwrapped states
    auto vkCommandBuffers = ALLOCA_ARRAY(VkCommandBuffer, commandBufferCount);

    // Unwrap and release wrappers
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        // Null destruction is allowed by the standard
        if (!pCommandBuffers[i]) {
            vkCommandBuffers[i] = nullptr;
            continue;
        }

        // Set native
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
