#pragma once

// Test
#include <Test/Device/IDevice.h>

// Std
#include <vector>

// Layer
#include <Backends/DX12/DX12.h>

// System
#include <wrl/client.h>

namespace Test::DX12 {
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
        PipelineID CreateComputePipeline(const ResourceLayoutID *layouts, uint32_t layoutCount, const void *shaderCode, uint32_t shaderSize) override;
        CommandBufferID CreateCommandBuffer(QueueType type) override;
        void BeginCommandBuffer(CommandBufferID commandBuffer) override;
        void EndCommandBuffer(CommandBufferID commandBuffer) override;
        void BindPipeline(CommandBufferID commandBuffer, PipelineID pipeline) override;
        void BindResourceSet(CommandBufferID commandBuffer, ResourceSetID resourceSet) override;
        void Dispatch(CommandBufferID commandBuffer, uint32_t x, uint32_t y, uint32_t z) override;
        void Submit(QueueID queue, CommandBufferID commandBuffer) override;
        void InitializeResources(CommandBufferID commandBuffer) override;
        void Flush() override;

    private:
        /// Creators
        void CreateDevice();
        void CreateCommandQueue();
        void CreateSharedQueues();
        void CreateSharedHeaps();
        void CreateSharedFence();

    private:
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        /// Hardware adapter
        ComPtr<IDXGIAdapter1> adapter;

        /// Graphics device
        ComPtr<ID3D12Device> device;

        /// Shared allocator
        ComPtr<ID3D12CommandAllocator> commandAllocator;

        /// Shared queues
        ComPtr<ID3D12CommandQueue> graphicsQueue;
        ComPtr<ID3D12CommandQueue> computeQueue;
        ComPtr<ID3D12CommandQueue> copyQueue;

        /// Flush fence
        ComPtr<ID3D12Fence> waitFence;

        /// Kernel fence event
        HANDLE waitFenceEvent;

        /// Current wait fence counter
        uint32_t waitFenceCounter{0};

        /// Shared descriptor heap
        ComPtr<ID3D12DescriptorHeap> sharedHeap;

        /// Shared heap descriptor handle stride
        size_t sharedHeapStride{0};

        /// Shared descriptor heap offsets, incremented
        D3D12_CPU_DESCRIPTOR_HANDLE sharedCPUHeapOffset{0};
        D3D12_GPU_DESCRIPTOR_HANDLE sharedGPUHeapOffset{0};

    private:
        struct ResourceInfo {
            ResourceType type;
            Backend::IL::Format format;
            ComPtr<ID3D12Resource> resource;
        };

        struct ResourceLayoutInfo {
            std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
        };

        struct ResourceSetInfo {
            D3D12_CPU_DESCRIPTOR_HANDLE heapCPUHandleOffset{0};
            D3D12_GPU_DESCRIPTOR_HANDLE heapGPUHandleOffset{0};
        };

        struct CommandBufferInfo {
            ComPtr<ID3D12GraphicsCommandList> commandList;

            struct Context {
                PipelineID pipeline;
            } context;
        };

        struct PipelineInfo {
            ComPtr<ID3D12PipelineState> pipeline;
            ComPtr<ID3D12RootSignature> rootSignature;
        };

        /// All objects
        std::vector<ResourceInfo> resources;
        std::vector<ResourceLayoutInfo> resourceLayouts;
        std::vector<ResourceSetInfo> resourceSets;
        std::vector<CommandBufferInfo> commandBuffers;
        std::vector<PipelineInfo> pipelines;
    };
}
