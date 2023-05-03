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
