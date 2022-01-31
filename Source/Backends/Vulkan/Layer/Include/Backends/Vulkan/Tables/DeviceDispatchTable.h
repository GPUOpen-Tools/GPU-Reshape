#pragma once

// Layer
#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VMA.h"
#include "Backends/Vulkan/TrackedObject.h"
#include "Backends/Vulkan/DependentObject.h"

// Common
#include "Common/Allocators.h"
#include "Common/Containers/ObjectPool.h"

// Generated
#include "Backends/Vulkan/CommandBufferDispatchTable.Gen.h"

// Std
#include <mutex>
#include <map>

// Forward declarations
struct InstanceDispatchTable;
struct CommandPoolState;
struct ShaderModuleState;
struct PipelineState;
struct PipelineLayoutState;
struct FenceState;
struct QueueState;
class Registry;
class IFeature;
class IBridge;
class InstrumentationController;
class ShaderExportStreamer;
class ShaderExportDescriptorAllocator;

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
    void Populate(PFN_vkGetInstanceProcAddr getInstanceProcAddr, PFN_vkGetDeviceProcAddr getDeviceProcAddr);

    /// Get the hook address for a given name
    /// \param name the name to hook
    /// \return the hooked address, may be nullptr
    static PFN_vkVoidFunction GetHookAddress(const char *name);

    /// States
    VkDevice object;
    VkPhysicalDevice physicalDevice;

    /// Parent table
    InstanceDispatchTable* parent{nullptr};

    /// Allocators
    Allocators allocators;

    /// Shared registry
    Registry* registry;

    /// Message bridge
    IBridge* bridge;

    /// Tracked objects
    TrackedObject<VkCommandPool, CommandPoolState>       states_commandPool;
    TrackedObject<VkShaderModule, ShaderModuleState>     states_shaderModule;
    TrackedObject<VkPipelineLayout, PipelineLayoutState> states_pipelineLayout;
    TrackedObject<VkFence, FenceState>                   states_fence;
    TrackedObject<VkQueue, QueueState>                   states_queue;
    TrackedObject<VkPipeline, PipelineState>             states_pipeline;

    /// Dependency objects
    DependentObject<ShaderModuleState, PipelineState> dependencies_shaderModulesPipelines;

    /// Controllers
    InstrumentationController* instrumentationController{nullptr};

    /// Export streamer
    ShaderExportStreamer* exportStreamer{nullptr};
    ShaderExportDescriptorAllocator* exportDescriptorAllocator{nullptr};

    /// Callbacks
    PFN_vkGetInstanceProcAddr             next_vkGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr               next_vkGetDeviceProcAddr;
    PFN_vkDestroyDevice                   next_vkDestroyDevice;
    PFN_vkCreateCommandPool               next_vkCreateCommandPool;
    PFN_vkAllocateCommandBuffers          next_vkAllocateCommandBuffers;
    PFN_vkBeginCommandBuffer              next_vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer                next_vkEndCommandBuffer;
    PFN_vkFreeCommandBuffers              next_vkFreeCommandBuffers;
    PFN_vkDestroyCommandPool              next_vkDestroyCommandPool;
    PFN_vkQueueSubmit                     next_vkQueueSubmit;
    PFN_vkQueuePresentKHR                 next_vkQueuePresentKHR;
    PFN_vkCreateShaderModule              next_vkCreateShaderModule;
    PFN_vkDestroyShaderModule             next_vkDestroyShaderModule;
    PFN_vkCreateGraphicsPipelines         next_vkCreateGraphicsPipelines;
    PFN_vkCreateComputePipelines          next_vkCreateComputePipelines;
    PFN_vkDestroyPipeline                 next_vkDestroyPipeline;
    PFN_vkGetFenceStatus                  next_vkGetFenceStatus;
    PFN_vkCreateBuffer                    next_vkCreateBuffer;
    PFN_vkDestroyBuffer                   next_vkDestroyBuffer;
    PFN_vkCreateBufferView                next_vkCreateBufferView;
    PFN_vkDestroyBufferView               next_vkDestroyBufferView;
    PFN_vkGetBufferMemoryRequirements     next_vkGetBufferMemoryRequirements;
    PFN_vkCreateDescriptorPool            next_vkCreateDescriptorPool;
    PFN_vkCreateDescriptorSetLayout       next_vkCreateDescriptorSetLayout;
    PFN_vkAllocateDescriptorSets          next_vkAllocateDescriptorSets;
    PFN_vkFreeDescriptorSets              next_vkFreeDescriptorSets;
    PFN_vkCreatePipelineLayout            next_vkCreatePipelineLayout;
    PFN_vkDestroyPipelineLayout           next_vkDestroyPipelineLayout;
    PFN_vkQueueWaitIdle                   next_vkQueueWaitIdle;
    PFN_vkDeviceWaitIdle                  next_vkDeviceWaitIdle;
    PFN_vkCreateFence                     next_vkCreateFence;
    PFN_vkDestroyFence                    next_vkDestroyFence;
    PFN_vkResetFences                     next_vkResetFences;
    PFN_vkUpdateDescriptorSets            next_vkUpdateDescriptorSets;
    PFN_vkBindBufferMemory                next_vkBindBufferMemory;
    PFN_vkGetDeviceQueue                  next_vkGetDeviceQueue;
    PFN_vkResetCommandBuffer              next_vkResetCommandBuffer;
    PFN_vkCreateImage                     next_vkCreateImage;
    PFN_vkDestroyImage                    next_vkDestroyImage;
    PFN_vkAllocateMemory                  next_vkAllocateMemory;
    PFN_vkFreeMemory                      next_vkFreeMemory;
    PFN_vkBindBufferMemory2KHR            next_vkBindBufferMemory2KHR;
    PFN_vkBindImageMemory                 next_vkBindImageMemory;
    PFN_vkBindImageMemory2KHR             next_vkBindImageMemory2KHR;
    PFN_vkFlushMappedMemoryRanges         next_vkFlushMappedMemoryRanges;
    PFN_vkGetBufferMemoryRequirements2KHR next_vkGetBufferMemoryRequirements2KHR;
    PFN_vkGetImageMemoryRequirements      next_vkGetImageMemoryRequirements;
    PFN_vkGetImageMemoryRequirements2KHR  next_vkGetImageMemoryRequirements2KHR;
    PFN_vkInvalidateMappedMemoryRanges    next_vkInvalidateMappedMemoryRanges;
    PFN_vkMapMemory                       next_vkMapMemory;
    PFN_vkUnmapMemory                     next_vkUnmapMemory;

    /// Properties
    VkPhysicalDeviceProperties physicalDeviceProperties{};
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures{};
    VkPhysicalDeviceDescriptorIndexingFeatures physicalDeviceDescriptorIndexingFeatures{};

    /// Command buffer dispatch table
    std::mutex                 commandBufferMutex;
    CommandBufferDispatchTable commandBufferDispatchTable;

    /// All features
    std::vector<IFeature*> features;

private:
    /// Lookup
    static std::mutex                              Mutex;
    static std::map<void *, DeviceDispatchTable *> Table;
};
