// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
        const char *GetName() override;
        void Install(const DeviceInfo &info) override;
        QueueID GetQueue(QueueType type) override;
        BufferID CreateTexelBuffer(ResourceType type, Backend::IL::Format format, uint64_t size, const void *data, uint64_t dataSize) override;
        TextureID CreateTexture(ResourceType type, Backend::IL::Format format, uint32_t width, uint32_t height, uint32_t depth, const void *data, uint64_t dataSize) override;
        ResourceLayoutID CreateResourceLayout(const ResourceType *types, uint32_t count, bool isLastUnbounded = false) override;
        ResourceSetID CreateResourceSet(ResourceLayoutID layout, const ResourceID *resources, uint32_t count) override;
        PipelineID CreateComputePipeline(const ResourceLayoutID *layouts, uint32_t layoutCount, const void *shaderCode, uint64_t shaderSize) override;
        CommandBufferID CreateCommandBuffer(QueueType type) override;
        void BeginCommandBuffer(CommandBufferID commandBuffer) override;
        void EndCommandBuffer(CommandBufferID commandBuffer) override;
        void BindPipeline(CommandBufferID commandBuffer, PipelineID pipeline) override;
        void BindResourceSet(CommandBufferID commandBuffer, uint32_t slot, ResourceSetID resourceSet) override;
        void Dispatch(CommandBufferID commandBuffer, uint32_t x, uint32_t y, uint32_t z) override;
        void Submit(QueueID queue, CommandBufferID commandBuffer) override;
        void InitializeResources(CommandBufferID commandBuffer) override;
        void Flush() override;
        SamplerID CreateSampler() override;
        CBufferID CreateCBuffer(uint32_t byteSize, const void *data, uint64_t dataSize) override;

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

    private:
        struct HeapInfo {
            /// Shared descriptor heap
            ComPtr<ID3D12DescriptorHeap> sharedHeap;

            /// Shared heap descriptor handle stride
            size_t sharedHeapStride{0};

            /// Shared descriptor heap offsets, incremented
            D3D12_CPU_DESCRIPTOR_HANDLE sharedCPUHeapOffset{0};
            D3D12_GPU_DESCRIPTOR_HANDLE sharedGPUHeapOffset{0};
        };

        /// Heaps
        HeapInfo sharedResourceHeap;
        HeapInfo sharedSamplerHeap;

        /// Get the heap for a given resource type
        HeapInfo& GetHeapForType(ResourceType type);

        /// Create a new shared heap
        /// \param heap given type
        /// \param count number of descriptors
        /// \param info output heap
        void CreateSharedHeap(D3D12_DESCRIPTOR_HEAP_TYPE heap, uint32_t count, HeapInfo& info);

    private:
        struct ResourceInfo {
            ResourceType type;
            Backend::IL::Format format;
            ComPtr<ID3D12Resource> resource;
        };

        struct ResourceLayoutInfo {
            std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
            std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
        };

        struct ResourceSetInfo {
            D3D12_CPU_DESCRIPTOR_HANDLE heapCPUHandleOffset{0};
            D3D12_GPU_DESCRIPTOR_HANDLE heapGPUHandleOffset{0};
            uint32_t count{0};
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

    private:
        /// Type of update
        enum class UpdateCommandType {
            CopyBuffer
        };

        /// Update payload
        union UpdateCommand {
            UpdateCommand() {

            }

            /// Type of command
            UpdateCommandType type;

            /// Buffer copy command
            struct {
                UpdateCommandType type;
                ID3D12Resource* dest;
                ID3D12Resource* source;
                uint64_t dataSize;
            } copyBuffer;
        };

        /// Queued initialization commands
        std::vector<UpdateCommand> updateCommands;

    private:
        struct UploadBuffer {
            ComPtr<ID3D12Resource> resource;
        };

        /// Create an upload buffer
        /// \param size expected size
        /// \return buffer
        UploadBuffer& CreateUploadBuffer(uint64_t size);

        /// Lazy pool of buffers
        std::vector<UploadBuffer> uploadBuffers;
    };
}
