#pragma once

// Layer
#include "Vulkan.h"

// Forward declarations
struct DeviceDispatchTable;

// Create a queue state
void CreateQueueState(DeviceDispatchTable* table, VkQueue queue, uint32_t familyIndex);

// Hooks
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkQueueWaitIdle(VkQueue queue);
VKAPI_ATTR VkResult VKAPI_CALL Hook_vkDeviceWaitIdle(VkDevice device);
