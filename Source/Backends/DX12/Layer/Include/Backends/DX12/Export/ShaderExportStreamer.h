#pragma once

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Layer
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/Export/ShaderExportStreamState.h>

// Common
#include <Common/Containers/ObjectPool.h>
#include <Common/Containers/TrivialObjectPool.h>

// Std
#include <mutex>

// Forward declarations
class ShaderExportDescriptorAllocator;
class ShaderExportStreamAllocator;
class DeviceAllocator;
struct DeviceDispatchTable;
struct PipelineState;
struct FenceState;
struct CommandQueueState;
struct FenceTable;
struct DeviceState;
struct CommandListState;
struct DescriptorHeapState;
struct CommandQueueTable;
class IBridge;

class ShaderExportStreamer : public TComponent<ShaderExportStreamer> {
public:
    COMPONENT(ShaderExportStreamer);

    ShaderExportStreamer(DeviceState* device);
    ~ShaderExportStreamer();

    /// Install this streamer
    /// \return success state
    bool Install();

    /// Allocate a new queue state
    /// \param state the given queue
    /// \return new queue state
    ShaderExportQueueState* AllocateQueueState(ID3D12CommandQueue* queue);

    /// Allocate a new stream state
    /// \return the stream state
    ShaderExportStreamState* AllocateStreamState();

    /// Allocate a new submission segment
    /// \return the segment
    ShaderExportStreamSegment* AllocateSegment();

    /// Free a stream state
    /// \param state the state
    void Free(ShaderExportStreamState* state);

    /// Free a queue state
    /// \param state the queue state
    void Free(ShaderExportQueueState* state);

    /// Record a patch command buffer for submissions
    /// \param queueState the queue to record for
    /// \param state the segment state
    /// \return command buffer to be submitted
    ID3D12GraphicsCommandList* RecordPatchCommandBuffer(CommandQueueState* queueState, ShaderExportStreamSegment* state);

    /// Enqueue a submitted segment
    /// \param queue the queue state that was submitted on
    /// \param segment the submitted segment
    /// \param fence shared submission fence, must be valid
    void Enqueue(CommandQueueState* queueState, ShaderExportStreamSegment* segment);

public:
    /// Invoked during command buffer recording
    /// \param state the stream state
    /// \param commandBuffer the command buffer
    void BeginCommandBuffer(ShaderExportStreamState* state, CommandListState* commandList);

    /// Invoked during pipeline binding
    /// \param state the stream state
    /// \param pipeline the pipeline state being bound
    /// \param instrumented true if an instrumented pipeline has been bound
    /// \param commandBuffer the command buffer
    void SetDescriptorHeap(ShaderExportStreamState* state, DescriptorHeapState* heap);

    /// Invoked during pipeline binding
    /// \param state the stream state
    /// \param pipeline the pipeline state being bound
    /// \param instrumented true if an instrumented pipeline has been bound
    /// \param commandBuffer the command buffer
    void SetRootSignature(ShaderExportStreamState* state, const RootSignatureState* rootSignature);

    /// Invoked during pipeline binding
    /// \param state the stream state
    /// \param pipeline the pipeline state being bound
    /// \param instrumented true if an instrumented pipeline has been bound
    /// \param commandBuffer the command buffer
    void BindPipeline(ShaderExportStreamState* state, const PipelineState* pipeline, bool instrumented, CommandListState* list);

    /// Map a stream state pre submission
    /// \param state the stream state
    /// \param segment the segment to be mapped to
    void MapSegment(ShaderExportStreamState* state, ShaderExportStreamSegment* segment);

public:
    /// Whole device sync point
    void Process();

    /// Queue specific sync point
    /// \param queueState the queue state
    void Process(CommandQueueState* queueState);

private:
    /// Bind the shader export for a pipeline
    /// \param state the stream state
    /// \param pipeline the pipeline to bind for
    /// \param commandBuffer the command buffer
    void BindShaderExport(ShaderExportStreamState* state, const PipelineState* pipeline, CommandListState* commandList);

    /// Process all segments within a queue
    /// \param queue the queue state
    void ProcessSegmentsNoQueueLock(CommandQueueState* queue);

    /// Process a segment
    bool ProcessSegment(ShaderExportStreamSegment* segment);

    /// Free a segment
    void FreeSegmentNoQueueLock(CommandQueueState* queue, ShaderExportStreamSegment* segment);

private:
    DeviceState* device;

    /// Internal mutex
    std::mutex mutex;

    /// Shared offset allocator
    BucketPoolAllocator<uint32_t> dynamicOffsetAllocator;

    /// Shared heaps
    ID3D12DescriptorHeap* sharedCPUHeap{ nullptr };
    ID3D12DescriptorHeap* sharedGPUHeap{ nullptr };

    /// Shared allocators
    ShaderExportDescriptorAllocator* sharedCPUHeapAllocator{ nullptr };
    ShaderExportDescriptorAllocator* sharedGPUHeapAllocator{nullptr};

    /// All pools
    ObjectPool<ShaderExportStreamState> streamStatePool;
    ObjectPool<ShaderExportStreamSegment> segmentPool;
    ObjectPool<ShaderExportQueueState> queuePool;

    /// Components
    ComRef<DeviceAllocator> deviceAllocator{nullptr};
    ComRef<ShaderExportStreamAllocator> streamAllocator{nullptr};
    ComRef<IBridge> bridge{nullptr};
};
