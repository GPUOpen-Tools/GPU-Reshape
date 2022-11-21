#pragma once

#include <Backends/Vulkan/Vulkan.h>

/// Hooks
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler);
VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyBufferView(VkDevice device, VkBufferView view, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR void VKAPI_CALL Hook_vkDestroyImageView(VkDevice device, VkImageView view, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR void VKAPI_CALL Hook_vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator);
