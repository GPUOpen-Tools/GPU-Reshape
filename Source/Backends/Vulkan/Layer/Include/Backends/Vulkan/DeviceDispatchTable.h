#pragma once

// Layer
#include "Vulkan.h"

// Generated
#include <Backends/Vulkan/CommandBufferDispatchTable.Gen.h>

// Std
#include <mutex>
#include <map>

struct DeviceDispatchTable {
    /// Add a new table
    /// \param key the given dispatch key
    /// \param table the table to be added
    static DeviceDispatchTable* Add(void *key, DeviceDispatchTable *table) {
        std::lock_guard<std::mutex> lock(Mutex);
        Table[key] = table;
        return table;
    }

    /// Get a table
    /// \param key the dispatch key
    /// \return the table
    static DeviceDispatchTable *Get(void *key) {
        if (!key) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(Mutex);
        return Table.at(key);
    }

    /// Populate this table
    /// \param getProcAddr the device proc address fn for the next layer
    void Populate(VkDevice device, PFN_vkGetInstanceProcAddr getInstanceProcAddr, PFN_vkGetDeviceProcAddr getDeviceProcAddr);

    /// Get the hook address for a given name
    /// \param name the name to hook
    /// \return the hooked address, may be nullptr
    static PFN_vkVoidFunction GetHookAddress(const char *name);

    /// States
    VkDevice object;

    /// Callbacks
    PFN_vkGetInstanceProcAddr next_vkGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr   next_vkGetDeviceProcAddr;
    PFN_vkDestroyDevice       next_vkDestroyDevice;

    /// Command buffer dispatch table
    CommandBufferDispatchTable commandBufferDispatchTable;

private:
    /// Lookup
    static std::mutex                              Mutex;
    static std::map<void *, DeviceDispatchTable *> Table;
};
