#pragma once

#include <Backends/Vulkan/Vulkan.h>

// Forward declarations
struct InstanceDispatchTable;

/// Invoke a bridge sync point
/// \param table instance table
void BridgeSyncPoint(InstanceDispatchTable* table);

/// Hooks
VkResult VKAPI_PTR Hook_vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties);
VkResult VKAPI_PTR Hook_vkEnumerateInstanceExtensionProperties(uint32_t *pPropertyCount, VkExtensionProperties *pProperties);
VkResult VKAPI_PTR Hook_vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance);
void     VKAPI_PTR Hook_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator);
