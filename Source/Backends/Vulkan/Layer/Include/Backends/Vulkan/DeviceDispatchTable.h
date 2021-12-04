#pragma once

// Layer
#include "Vulkan.h"
#include "TrackedObject.h"
#include "DependentObject.h"

// Common
#include <Common/Allocators.h>

// Generated
#include <Backends/Vulkan/CommandBufferDispatchTable.Gen.h>

// Std
#include <mutex>
#include <map>

// Forward declarations
class InstrumentationController;
struct CommandPoolState;
struct ShaderModuleState;
struct PipelineState;
class Registry;
class IFeature;
class IBridge;
struct InstanceDispatchTable;

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

    /// Parent table
    InstanceDispatchTable* parent{nullptr};

    /// Allocators
    Allocators allocators;

    /// Shared registry
    Registry* registry;

    /// Message bridge
    IBridge* bridge;

    /// Tracked objects
    TrackedObject<VkCommandPool, CommandPoolState>   states_commandPool;
    TrackedObject<VkShaderModule, ShaderModuleState> states_shaderModule;
    TrackedObject<VkPipeline, PipelineState>         states_pipeline;

    /// Dependency objects
    DependentObject<ShaderModuleState, PipelineState> dependencies_shaderModulesPipelines;

    /// Controllers
    InstrumentationController* instrumentationController;

    /// Callbacks
    PFN_vkGetInstanceProcAddr     next_vkGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr       next_vkGetDeviceProcAddr;
    PFN_vkDestroyDevice           next_vkDestroyDevice;
    PFN_vkCreateCommandPool       next_vkCreateCommandPool;
    PFN_vkAllocateCommandBuffers  next_vkAllocateCommandBuffers;
    PFN_vkBeginCommandBuffer      next_vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer        next_vkEndCommandBuffer;
    PFN_vkFreeCommandBuffers      next_vkFreeCommandBuffers;
    PFN_vkDestroyCommandPool      next_vkDestroyCommandPool;
    PFN_vkQueueSubmit             next_vkQueueSubmit;
    PFN_vkCreateShaderModule      next_vkCreateShaderModule;
    PFN_vkDestroyShaderModule     next_vkDestroyShaderModule;
    PFN_vkCreateGraphicsPipelines next_vkCreateGraphicsPipelines;
    PFN_vkCreateComputePipelines  next_vkCreateComputePipelines;
    PFN_vkDestroyPipeline         next_vkDestroyPipeline;
    PFN_vkCmdBindPipeline         next_vkCmdBindPipeline;

    /// Command buffer dispatch table
    std::mutex                 commandBufferMutex;
    CommandBufferDispatchTable commandBufferDispatchTable;

private:
    /// Lookup
    static std::mutex                              Mutex;
    static std::map<void *, DeviceDispatchTable *> Table;
};
