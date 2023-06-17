// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/Instance.h>
#include <Backends/Vulkan/Device.h>

// Std
#include <cstring>

/// Global instantiation
std::mutex                                InstanceDispatchTable::Mutex;
std::map<void *, InstanceDispatchTable *> InstanceDispatchTable::Table;

void InstanceDispatchTable::Populate(VkInstance instance, PFN_vkGetInstanceProcAddr getProcAddr) {
    object                     = instance;
    next_vkGetInstanceProcAddr = getProcAddr;

    // Populate instance
    next_vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(getProcAddr(instance, "vkDestroyInstance"));
    next_vkGetPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(getProcAddr(object, "vkGetPhysicalDeviceMemoryProperties"));
    next_vkGetPhysicalDeviceMemoryProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties2KHR>(getProcAddr(object, "vkGetPhysicalDeviceMemoryProperties2KHR"));
    next_vkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(getProcAddr(object, "vkGetPhysicalDeviceProperties"));
    next_vkGetPhysicalDeviceFeatures2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(getProcAddr(object, "vkGetPhysicalDeviceFeatures2"));
    next_vkEnumerateDeviceLayerProperties = reinterpret_cast<PFN_vkEnumerateDeviceLayerProperties>(getProcAddr(object, "vkEnumerateDeviceLayerProperties"));
    next_vkEnumerateDeviceExtensionProperties = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(getProcAddr(object, "vkEnumerateDeviceExtensionProperties"));
    next_vkGetPhysicalDeviceQueueFamilyProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(getProcAddr(object, "vkGetPhysicalDeviceQueueFamilyProperties"));
}

PFN_vkVoidFunction InstanceDispatchTable::GetHookAddress(const char *name) {
    if (!std::strcmp(name, "vkCreateInstance"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkCreateInstance);

    if (!std::strcmp(name, "vkDestroyInstance"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkDestroyInstance);

    if (!std::strcmp(name, "vkEnumerateInstanceLayerProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkEnumerateInstanceLayerProperties);

    if (!std::strcmp(name, "vkEnumerateInstanceExtensionProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkEnumerateInstanceExtensionProperties);

    if (!std::strcmp(name, "vkEnumerateDeviceLayerProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkEnumerateDeviceLayerProperties);

    if (!std::strcmp(name, "vkEnumerateDeviceExtensionProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(&Hook_vkEnumerateDeviceExtensionProperties);

    // No hook
    return nullptr;
}
