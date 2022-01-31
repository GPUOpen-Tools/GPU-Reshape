#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>

// Backend
#include <Backend/Environment.h>

// Common
#include <Common/Allocators.h>

// Bridge
#include <Bridge/Log/LogBuffer.h>

// Std
#include <mutex>
#include <vector>
#include <map>

// Forward declarations
class Registry;
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

    /// Allocators
    Allocators allocators;

    /// Shared registry
    Registry* registry;

    /// Message bridge
    IBridge* bridge;

    /// Shared logging buffer
    LogBuffer logBuffer;

    /// Optional environment, ignored if creation parameters supply a registry
    Backend::Environment environment;

    /// Callbacks
    PFN_vkGetInstanceProcAddr                   next_vkGetInstanceProcAddr;
    PFN_vkDestroyInstance                       next_vkDestroyInstance;
    PFN_vkGetPhysicalDeviceMemoryProperties     next_vkGetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties2KHR next_vkGetPhysicalDeviceMemoryProperties2KHR;
    PFN_vkGetPhysicalDeviceProperties           next_vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceFeatures2            next_vkGetPhysicalDeviceFeatures2;

private:
    /// Lookup
    static std::mutex                                Mutex;
    static std::map<void *, InstanceDispatchTable *> Table;
};
