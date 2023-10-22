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

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>

// Backend
#include <Backend/Environment.h>

// Common
#include <Common/Allocators.h>
#include <Common/ComRef.h>
#include <Common/Registry.h>

// Bridge
#include <Bridge/Log/LogBuffer.h>

// Std
#include <mutex>
#include <vector>
#include <map>

// Forward declarations
class IFeature;

struct InstanceDispatchTable {
    /// Add a new table
    /// \param key the given dispatch key
    /// \param table the table to be added
    static InstanceDispatchTable* Add(void *key, InstanceDispatchTable *table) {
        std::lock_guard<std::mutex> lock(Mutex);
        Table[key] = table;
        return table;
    }

    /// Get a table
    /// \param key the dispatch key
    /// \return the table
    static InstanceDispatchTable *Get(void *key) {
        if (!key) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(Mutex);
        return Table.at(key);
    }

    /// Populate this table
    /// \param getProcAddr the device proc address fn for the next layer
    void Populate(VkInstance instance, PFN_vkGetInstanceProcAddr getProcAddr);

    /// Get the hook address for a given name
    /// \param name the name to hook
    /// \return the hooked address, may be nullptr
    static PFN_vkVoidFunction GetHookAddress(const char *name);

    /// States
    VkInstance object;

    /// Creation deep copy
    VkInstanceCreateInfoDeepCopy createInfo;

    /// Allocators
    Allocators allocators;

    /// Shared registry
    Registry registry;

    /// Message bridge
    ComRef<IBridge> bridge;

    /// Shared logging buffer
    LogBuffer logBuffer;

    /// Optional environment, ignored if creation parameters supply a registry
    Backend::Environment environment;

    /// Creation info
    VkApplicationInfoDeepCopy applicationInfo;

    /// Callbacks
    PFN_vkGetInstanceProcAddr                    next_vkGetInstanceProcAddr;
    PFN_vkDestroyInstance                        next_vkDestroyInstance;
    PFN_vkGetPhysicalDeviceMemoryProperties      next_vkGetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties2KHR  next_vkGetPhysicalDeviceMemoryProperties2KHR;
    PFN_vkGetPhysicalDeviceProperties            next_vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceFeatures2             next_vkGetPhysicalDeviceFeatures2;
    PFN_vkEnumerateDeviceLayerProperties         next_vkEnumerateDeviceLayerProperties;
    PFN_vkEnumerateDeviceExtensionProperties     next_vkEnumerateDeviceExtensionProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties next_vkGetPhysicalDeviceQueueFamilyProperties;

private:
    /// Lookup
    static std::mutex                                Mutex;
    static std::map<void *, InstanceDispatchTable *> Table;
};
