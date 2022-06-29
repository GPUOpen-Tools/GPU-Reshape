#pragma once

#include <Backends/Vulkan/Vulkan.h>

// Forward declarations
struct DeviceDispatchTable;

/// Invoke a bridge sync point
/// \param table instance table
void BridgeDeviceSyncPoint(DeviceDispatchTable* table);

/// Hooks
VkResult VKAPI_PTR Hook_vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties);
VkResult VKAPI_PTR Hook_vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties);
VkResult VKAPI_PTR Hook_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice);
void     VKAPI_PTR Hook_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator);
