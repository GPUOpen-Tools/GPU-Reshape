// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
struct InstanceDispatchTable;

/// Invoke a bridge sync point
/// \param table instance table
void BridgeInstanceSyncPoint(InstanceDispatchTable* table);

/// Hooks
VkResult VKAPI_PTR Hook_vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount, VkLayerProperties *pProperties);
VkResult VKAPI_PTR Hook_vkEnumerateInstanceExtensionProperties(uint32_t *pPropertyCount, VkExtensionProperties *pProperties);
VkResult VKAPI_PTR Hook_vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance);
void     VKAPI_PTR Hook_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator);
