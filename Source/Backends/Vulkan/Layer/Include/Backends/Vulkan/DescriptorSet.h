#pragma once

#include <Backends/Vulkan/Vulkan.h>

/// Hooks
VKAPI_ATTR VkResult VKAPI_PTR Hook_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout);
VKAPI_ATTR void VKAPI_PTR Hook_vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout);
VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR void VKAPI_PTR Hook_vkCmdBindDescriptorSets(CommandBufferObject* commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets);
VKAPI_ATTR void VKAPI_CALL Hook_vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies);
