#pragma once

// Common
#include <Common/IComponent.h>

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/Export/StreamState.h>

// Common
#include <Common/Containers/ObjectPool.h>
#include <Common/Containers/TrivialObjectPool.h>

// Forward declarations
struct ShaderExportDescriptorAllocator;
struct ShaderExportStreamAllocator;
struct DeviceAllocator;
struct DeviceDispatchTable;
struct PipelineState;
struct FenceState;
struct QueueState;
class IBridge;

class ShaderExportStreamer : public IComponent {
public:
    COMPONENT(ShaderExportStreamer);

    ShaderExportStreamer(DeviceDispatchTable* table);

    bool Install();

    /// Allocate a new queue state
    /// \param state the given queue
    /// \return new queue state
    ShaderExportQueueState* AllocateQueueState(QueueState* state);

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
    VkCommandBuffer RecordPatchCommandBuffer(ShaderExportQueueState* queueState, ShaderExportStreamSegment* state);

    /// Enqueue a submitted segment
    /// \param queue the queue state that was submitted on
    /// \param segment the submitted segment
    /// \param fence shared submission fence, must be valid
    void Enqueue(ShaderExportQueueState* queue, ShaderExportStreamSegment* segment, FenceState* fence);

public:
    /// Invoked during command buffer recording
    /// \param state the stream state
    /// \param commandBuffer the command buffer
    void BeginCommandBuffer(ShaderExportStreamState* state, VkCommandBuffer commandBuffer);

    /// Invoked during pipeline binding
    /// \param state the stream state
    /// \param pipeline the pipeline state being bound
    /// \param instrumented true if an instrumented pipeline has been bound
    /// \param commandBuffer the command buffer
    void BindPipeline(ShaderExportStreamState* state, const PipelineState* pipeline, bool instrumented, VkCommandBuffer commandBuffer);

    /// Invoked during descriptor binding
    /// \param state the stream state
    /// \param bindPoint the bind point
    /// \param start the first set
    /// \param count the number of sets
    /// \param sets the sets to be bound
    void BindDescriptorSets(ShaderExportStreamState* state, VkPipelineBindPoint bindPoint, uint32_t start, uint32_t count, const VkDescriptorSet* sets);

    /// Map a stream state pre submission
    /// \param state the stream state
    /// \param segment the segment to be mapped to
    void MapSegment(ShaderExportStreamState* state, ShaderExportStreamSegment* segment);

public:
    /// Whole device sync point
    void SyncPoint();

    /// Queue specific sync point
    /// \param queueState the queue state
    void SyncPoint(ShaderExportQueueState* queueState);

private:
    /// Migrate the descriptor environment to a new pipeline state
    /// \param state the stream state
    /// \param pipeline the new pipeline
    /// \param commandBuffer the command buffer
    void MigrateDescriptorEnvironment(ShaderExportStreamState* state, const PipelineState* pipeline, VkCommandBuffer commandBuffer);

    /// Bind the shader export for a pipeline
    /// \param state the stream state
    /// \param pipeline the pipeline to bind for
    /// \param commandBuffer the command buffer
    void BindShaderExport(ShaderExportStreamState* state, const PipelineState* pipeline, VkCommandBuffer commandBuffer);

    /// Process all segments within a queue
    /// \param queue the queue state
    void ProcessSegmentsNoQueueLock(ShaderExportQueueState* queue);

    /// Process a segment
    bool ProcessSegment(ShaderExportStreamSegment* segment);

    /// Free a segment
    void FreeSegmentNoQueueLock(ShaderExportQueueState* queue, ShaderExportStreamSegment* segment);

private:
    DeviceDispatchTable* table;

    ObjectPool<ShaderExportStreamState> streamStatePool;
    ObjectPool<ShaderExportStreamSegment> segmentPool;
    ObjectPool<ShaderExportQueueState> queuePool;

    DeviceAllocator* deviceAllocator{nullptr};

    ShaderExportDescriptorAllocator* descriptorAllocator{nullptr};

    ShaderExportStreamAllocator* streamAllocator{nullptr};

    IBridge* bridge{nullptr};
};
