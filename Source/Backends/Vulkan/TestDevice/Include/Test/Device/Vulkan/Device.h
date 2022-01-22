#pragma once

#include <Test/Device/IDevice.h>

// Vulkan
#include <vulkan/vulkan_core.h>

// VMA
#include <VMA/vk_mem_alloc.h>

#include <vector>

namespace Test::Vulkan {
    class Device final : public IDevice {
    public:
        ~Device();

        /// Overrides
        void Install(const DeviceInfo &info) override;
        QueueID GetQueue(QueueType type) override;
        BufferID CreateTexelBuffer(ResourceType type, Backend::IL::Format format, uint64_t size, void *data) override;
        TextureID CreateTexture(ResourceType type, Backend::IL::Format format, uint32_t width, uint32_t height, uint32_t depth, void *data) override;
        ResourceLayoutID CreateResourceLayout(const ResourceType *types, uint32_t count) override;
        ResourceSetID CreateResourceSet(ResourceLayoutID layout, const ResourceID *resources, uint32_t count) override;
        PipelineID CreateComputePipeline(const ResourceLayoutID* layouts, uint32_t layoutCount, const void *shaderCode, uint32_t shaderSize) override;
        CommandBufferID CreateCommandBuffer(QueueType type) override;
        void BeginCommandBuffer(CommandBufferID commandBuffer) override;
        void EndCommandBuffer(CommandBufferID commandBuffer) override;
        void BindPipeline(CommandBufferID commandBuffer, PipelineID pipeline) override;
        void BindResourceSet(CommandBufferID commandBuffer, ResourceSetID resourceSet) override;
        void Dispatch(CommandBufferID commandBuffer, uint32_t x, uint32_t y, uint32_t z) override;
        void Submit(QueueID queue, CommandBufferID commandBuffer) override;
        void Flush() override;

    private:
        /// Creation utilities
        void EnumerateInstanceExtensions();
        void EnumerateDeviceExtensions();
        void CreateInstance();
        void CreateDevice();
        void CreateAllocator();
        void CreateDebugMessenger();
        void CreateSharedDescriptorPool();
        void CreateSharedQueuePools();

        /// Query utilities
        bool SupportsInstanceLayer(const char *name) const;
        bool SupportsInstanceExtension(const char *name) const;
        bool SupportsDeviceExtension(const char *name) const;

        /// Release utilities
        void ReleaseResources();
        void ReleaseShared();

    private:
        /// Static debug callback
        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData
        );

    private:
        /// Creation info
        DeviceInfo deviceInfo;

    private:
        /// Layers and extensions
        std::vector<VkExtensionProperties> instanceExtensions;
        std::vector<VkLayerProperties> instanceLayers;
        std::vector<VkExtensionProperties> deviceExtensions;
        std::vector<VkLayerProperties> deviceLayers;

    private:
        /// Shared objects
        VkInstance instance{VK_NULL_HANDLE};
        VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
        VkDevice device{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};

    private:
        struct QueueInfo {
            uint32_t family{~0u};
            VkQueue queue{VK_NULL_HANDLE};
            VkCommandPool sharedCommandPool;
        };

        /// Create a given queue pool
        void CreateSharedQueuePool(QueueInfo& info);

        /// All queues
        QueueInfo graphicsQueue;
        QueueInfo computeQueue;
        QueueInfo transferQueue;

    private:
        /// Shared allocator
        VmaAllocator allocator;

    private:
        union ResourceInfo {
            ResourceType type;

            struct {
                ResourceType type;
                VkBuffer buffer;
                VkBufferView view;
                VmaAllocation allocation;
            } texelBuffer;

            struct {
                ResourceType type;
                VkImage image;
                VkImageView view;
                VmaAllocation allocation;
            } texture;
        };

        struct ResourceLayoutInfo {
            std::vector<ResourceType> resources;
            VkDescriptorSetLayout layout;
        };

        struct ResourceSetInfo {
            VkDescriptorSet set;
        };

        struct CommandBufferInfo {
            VkCommandBuffer commandBuffer;
            VkCommandPool pool;

            struct Context {
                PipelineID pipeline;
            } context;
        };

        struct PipelineInfo {
            VkPipeline pipeline;
            VkPipelineLayout layout;
        };

        /// All objects
        std::vector<ResourceInfo> resources;
        std::vector<ResourceLayoutInfo> resourceLayouts;
        std::vector<ResourceSetInfo> resourceSets;
        std::vector<CommandBufferInfo> commandBuffers;
        std::vector<PipelineInfo> pipelines;

    private:
        /// Shared descriptor pool
        VkDescriptorPool sharedDescriptorPool;
    };
}
