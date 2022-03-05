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
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkResetCommandBuffer(CommandBufferObject *commandBuffer, VkCommandBufferResetFlags flags);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkEndCommandBuffer(CommandBufferObject* commandBuffer);
VKAPI_ATTR void     VKAPI_CALL Hook_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, CommandBufferObject** const pCommandBuffers);
VKAPI_ATTR void     VKAPI_CALL Hook_vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR void     VKAPI_CALL Hook_vkCmdBindPipeline(CommandBufferObject*  commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
