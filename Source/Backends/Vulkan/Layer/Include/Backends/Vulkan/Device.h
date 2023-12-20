// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
