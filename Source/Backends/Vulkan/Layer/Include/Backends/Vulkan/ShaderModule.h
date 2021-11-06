#pragma once

#include <Backends/Vulkan/Vulkan.h>

/// Hooks
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule);
VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator);
