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

#pragma once

// Vulkan
#include "Vulkan.h"

// Layer
#include <Backends/Vulkan/Objects/CommandBufferObject.h>

/// Create all device command proxies
/// \param table
void CreateDeviceCommandProxies(DeviceDispatchTable* table);

/// Set the feature set
/// \param table
/// \param featureSet
void SetDeviceCommandFeatureSetAndCommit(DeviceDispatchTable* table, uint64_t featureSet);

/// Hooks
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, CommandBufferObject** pCommandBuffers);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkBeginCommandBuffer(CommandBufferObject* commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdExecuteCommands(CommandBufferObject* commandBuffer, uint32_t commandBufferCount, const  CommandBufferObject** pCommandBuffers);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkResetCommandBuffer(CommandBufferObject *commandBuffer, VkCommandBufferResetFlags flags);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkResetCommandPool(VkDevice device, VkCommandPool pool, VkCommandPoolResetFlags flags);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkEndCommandBuffer(CommandBufferObject* commandBuffer);
VKAPI_ATTR void     VKAPI_CALL Hook_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, CommandBufferObject** const pCommandBuffers);
VKAPI_ATTR void     VKAPI_CALL Hook_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdBindPipeline(CommandBufferObject *commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdDraw(CommandBufferObject *commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdDrawIndexed(CommandBufferObject *commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdDrawIndirect(CommandBufferObject *commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdDrawIndexedIndirect(CommandBufferObject *commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdDispatch(CommandBufferObject *commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdDispatchBase(CommandBufferObject *commandBuffer, uint32_t baseCountX, uint32_t baseCountY, uint32_t baseCountZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdDispatchIndirect(CommandBufferObject *commandBuffer, VkBuffer buffer, VkDeviceSize offset);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdDrawIndirectCount(CommandBufferObject* commandBuffer, VkBuffer buffer, VkDeviceSize offset,VkBuffer countBuffer,VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdDrawIndexedIndirectCount(CommandBufferObject* commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdPushConstants(CommandBufferObject* commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdBeginRenderPass(CommandBufferObject* commandBuffer, const VkRenderPassBeginInfo* info, VkSubpassContents contents);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdEndRenderPass(CommandBufferObject* commandBuffer);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdPushDescriptorSetKHR(CommandBufferObject* commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdPushDescriptorSetWithTemplateKHR(CommandBufferObject* commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void* pData);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdWaitEvents(CommandBufferObject *commandBuffer, uint32_t eventCount, const VkEvent *pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdPipelineBarrier(CommandBufferObject *commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdSetEvent2(CommandBufferObject *commandBuffer, VkEvent event, const VkDependencyInfo *pDependencyInfo);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdWaitEvents2(CommandBufferObject *commandBuffer, uint32_t eventCount, const VkEvent *pEvents, const VkDependencyInfo *pDependencyInfos);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdPipelineBarrier2(CommandBufferObject *commandBuffer, const VkDependencyInfo *pDependencyInfo);
