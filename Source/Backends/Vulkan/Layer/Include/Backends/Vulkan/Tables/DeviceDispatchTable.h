#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/VMA.h>
#include <Backends/Vulkan/TrackedObject.h>
#include <Backends/Vulkan/DependentObject.h>
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>
#include <Backends/Vulkan/Resource/PhysicalResourceIdentifierMap.h>

// Common
#include "Common/Allocators.h"
#include "Common/ComRef.h"
#include "Common/Registry.h"
#include "Common/Containers/ObjectPool.h"

// Generated
#include "Backends/Vulkan/CommandBufferDispatchTable.Gen.h"

// Backend
#include <Backend/EventDataStack.h>

// Std
#include <mutex>
#include <map>

// Forward declarations
struct InstanceDispatchTable;
struct CommandPoolState;
struct ShaderModuleState;
struct DescriptorSetLayoutState;
struct DescriptorSetState;
struct DescriptorPoolState;
struct SamplerState;
struct BufferState;
struct SwapchainState;
struct BufferViewState;
struct ImageState;
struct ImageViewState;
struct SamplerState;
struct PipelineState;
struct PipelineLayoutState;
struct RenderPassState;
struct FenceState;
struct QueueState;
class IFeature;
class IBridge;
class InstrumentationController;
class FeatureController;
class MetadataController;
class ShaderExportStreamer;
class ShaderExportDescriptorAllocator;
class ShaderSGUIDHost;
class ShaderDataHost;
class PhysicalResourceMappingTable;
class ShaderProgramHost;

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

    /// Deep copy of creation info
    VkDeviceCreateInfoDeepCopy createInfo;

    /// States
    VkDevice object;
    VkPhysicalDevice physicalDevice;

    /// Parent table
    InstanceDispatchTable* parent{nullptr};

    /// Allocators
    Allocators allocators;

    /// Shared registry
    Registry registry;

    /// Message bridge
    ComRef<IBridge> bridge;

    /// Tracked objects
    TrackedObject<VkCommandPool, CommandPoolState>                 states_commandPool;
    TrackedObject<VkShaderModule, ShaderModuleState>               states_shaderModule;
    TrackedObject<VkDescriptorSetLayout, DescriptorSetLayoutState> states_descriptorSetLayout;
    TrackedObject<VkDescriptorSet, DescriptorSetState>             states_descriptorSet;
    TrackedObject<VkDescriptorPool, DescriptorPoolState>           states_descriptorPool;
    TrackedObject<VkSampler, SamplerState>                         states_sampler;
    TrackedObject<VkBuffer, BufferState>                           states_buffer;
    TrackedObject<VkSwapchainKHR, SwapchainState>                  states_swapchain;
    TrackedObject<VkBufferView, BufferViewState>                   states_bufferView;
    TrackedObject<VkImage, ImageState>                             states_image;
    TrackedObject<VkImageView, ImageViewState>                     states_imageView;
    TrackedObject<VkPipelineLayout, PipelineLayoutState>           states_pipelineLayout;
    TrackedObject<VkRenderPass, RenderPassState>                   states_renderPass;
    TrackedObject<VkFence, FenceState>                             states_fence;
    TrackedObject<VkQueue, QueueState>                             states_queue;
    TrackedObject<VkPipeline, PipelineState>                       states_pipeline;

    /// Dependency objects
    DependentObject<ShaderModuleState, PipelineState> dependencies_shaderModulesPipelines;

    /// Physical identifier map
    PhysicalResourceIdentifierMap physicalResourceIdentifierMap;

    /// Virtual to physical resource mapping table
    ComRef<PhysicalResourceMappingTable> prmTable{nullptr};

    /// User programs
    ComRef<ShaderProgramHost> shaderProgramHost{nullptr};

    /// Controllers
    ComRef<InstrumentationController> instrumentationController{nullptr};
    ComRef<FeatureController> featureController{nullptr};
    ComRef<MetadataController> metadataController{nullptr};

    /// User controllers
    ComRef<ShaderSGUIDHost> sguidHost{nullptr};
    ComRef<ShaderDataHost> dataHost{nullptr};

    /// Export streamer
    ComRef<ShaderExportStreamer> exportStreamer{nullptr};
    ComRef<ShaderExportDescriptorAllocator> exportDescriptorAllocator{nullptr};

    /// Callbacks
    PFN_vkGetInstanceProcAddr             next_vkGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr               next_vkGetDeviceProcAddr;
    PFN_vkDestroyDevice                   next_vkDestroyDevice;
    PFN_vkCreateCommandPool               next_vkCreateCommandPool;
    PFN_vkAllocateCommandBuffers          next_vkAllocateCommandBuffers;
    PFN_vkBeginCommandBuffer              next_vkBeginCommandBuffer;
    PFN_vkResetCommandBuffer              next_vkResetCommandBuffer;
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
    PFN_vkWaitForFences                   next_vkWaitForFences;
    PFN_vkCreateBuffer                    next_vkCreateBuffer;
    PFN_vkDestroyBuffer                   next_vkDestroyBuffer;
    PFN_vkCreateBufferView                next_vkCreateBufferView;
    PFN_vkDestroyBufferView               next_vkDestroyBufferView;
    PFN_vkGetBufferMemoryRequirements     next_vkGetBufferMemoryRequirements;
    PFN_vkCreateDescriptorPool            next_vkCreateDescriptorPool;
    PFN_vkDestroyDescriptorPool           next_vkDestroyDescriptorPool;
    PFN_vkCreateDescriptorSetLayout       next_vkCreateDescriptorSetLayout;
    PFN_vkDestroyDescriptorSetLayout      next_vkDestroyDescriptorSetLayout;
    PFN_vkResetDescriptorPool             next_vkResetDescriptorPool;
    PFN_vkAllocateDescriptorSets          next_vkAllocateDescriptorSets;
    PFN_vkFreeDescriptorSets              next_vkFreeDescriptorSets;
    PFN_vkCreatePipelineLayout            next_vkCreatePipelineLayout;
    PFN_vkDestroyPipelineLayout           next_vkDestroyPipelineLayout;
    PFN_vkQueueWaitIdle                   next_vkQueueWaitIdle;
    PFN_vkDeviceWaitIdle                  next_vkDeviceWaitIdle;
    PFN_vkCreateFence                     next_vkCreateFence;
    PFN_vkDestroyFence                    next_vkDestroyFence;
    PFN_vkResetFences                     next_vkResetFences;
    PFN_vkCreateRenderPass                next_vkCreateRenderPass;
    PFN_vkCreateRenderPass2               next_vkCreateRenderPass2;
    PFN_vkDestroyRenderPass               next_vkDestroyRenderPass;
    PFN_vkUpdateDescriptorSets            next_vkUpdateDescriptorSets;
    PFN_vkBindBufferMemory                next_vkBindBufferMemory;
    PFN_vkGetDeviceQueue                  next_vkGetDeviceQueue;
    PFN_vkCreateImage                     next_vkCreateImage;
    PFN_vkCreateImageView                 next_vkCreateImageView;
    PFN_vkDestroyImage                    next_vkDestroyImage;
    PFN_vkDestroyImageView                next_vkDestroyImageView;
    PFN_vkCreateSampler                   next_vkCreateSampler;
    PFN_vkDestroySampler                  next_vkDestroySampler;
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
    PFN_vkCreateSwapchainKHR              next_vkCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR             next_vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR           next_vkGetSwapchainImagesKHR;
    PFN_vkSetDebugUtilsObjectNameEXT      next_vkSetDebugUtilsObjectNameEXT;

    /// Properties
    VkPhysicalDeviceProperties                 physicalDeviceProperties{};
    VkPhysicalDeviceFeatures2                  physicalDeviceFeatures{};
    VkPhysicalDeviceDescriptorIndexingFeatures physicalDeviceDescriptorIndexingFeatures{};
    VkPhysicalDeviceRobustness2FeaturesEXT     physicalDeviceRobustness2Features{};

    /// Command buffer dispatch table
    std::mutex                 commandBufferMutex;
    CommandBufferDispatchTable commandBufferDispatchTable;

    /// Shared remapping table
    EventDataStack::RemappingTable eventRemappingTable;

    /// All features
    std::vector<ComRef<IFeature>> features;

private:
    /// Lookup
    static std::mutex                              Mutex;
    static std::map<void *, DeviceDispatchTable *> Table;
};
