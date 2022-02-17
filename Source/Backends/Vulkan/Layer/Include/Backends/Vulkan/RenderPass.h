#pragma once

#include <Backends/Vulkan/Vulkan.h>

/// Hooks
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator);
