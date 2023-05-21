#pragma once

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/Export/StreamState.h>
#include <Backends/Vulkan/Resource/DescriptorDataSegment.h>

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
struct QueueState;
struct CommandBufferObject;
struct DescriptorSetState;
class IBridge;

class ShaderExportStreamer : public TComponent<ShaderExportStreamer> {
public:
    COMPONENT(ShaderExportStreamer);

    ShaderExportStreamer(DeviceDispatchTable* table);

    ~ShaderExportStreamer();

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

    /// Invoked during command buffer resetting
    /// \param state the stream state
    void ResetCommandBuffer(ShaderExportStreamState* state);

    /// Invoked during command buffer closing
    /// \param state the stream state
    /// \param commandBuffer the command buffer
    void EndCommandBuffer(ShaderExportStreamState* state, VkCommandBuffer commandBuffer);

    /// Invoked during pipeline binding
    /// \param state the stream state
    /// \param pipeline the pipeline state being bound
    /// \param object the backend state being bound
    /// \param instrumented true if an instrumented pipeline has been bound
    /// \param commandBuffer the command buffer
    void BindPipeline(ShaderExportStreamState* state, const PipelineState* pipeline, VkPipeline object, bool instrumented, VkCommandBuffer commandBuffer);

    /// Invoked during descriptor binding
    /// \param state the stream state
    /// \param bindPoint the bind point
    /// \param start the first set
    /// \param count the number of sets
    /// \param sets the sets to be bound
    /// \param commandBuffer command buffer being invoked
    void BindDescriptorSets(ShaderExportStreamState* state, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t start, uint32_t count, const VkDescriptorSet* sets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets, VkCommandBuffer commandBuffer);

    /// Map a stream state pre submission
    /// \param state the stream state
    /// \param segment the segment to be mapped to
    void MapSegment(ShaderExportStreamState* state, ShaderExportStreamSegment* segment);

    /// Commit all data
    /// \param state state to be committed to
    /// \param bindPoint destination binding point
    /// \param commandBufferObject object committing from
    void Commit(ShaderExportStreamState* state, VkPipelineBindPoint bindPoint, VkCommandBuffer commandBufferObject);

    /// Bind the shader export for a pipeline
    /// \param state the stream state
    /// \param slot the slot to be bound to
    /// \param type the expected pipeline type
    /// \param commandBuffer the command buffer
    void BindShaderExport(ShaderExportStreamState* state, PipelineType type, VkPipelineLayout layout, VkPipeline pipeline, uint32_t prmtPushConstantOffset, uint32_t slot, VkCommandBuffer commandBuffer);

    /// Bind the shader export for a pipeline
    /// \param state the stream state
    /// \param pipeline the pipeline to bind for
    /// \param commandBuffer the command buffer
    void BindShaderExport(ShaderExportStreamState* state, const PipelineState* pipeline, VkCommandBuffer commandBuffer);

public:
    /// Whole device sync point
    void Process();

    /// Queue specific sync point
    /// \param queueState the queue state
    void Process(ShaderExportQueueState* queueState);

private:
    /// Migrate the descriptor environment to a new pipeline state
    /// \param state the stream state
    /// \param pipeline the new pipeline
    /// \param commandBuffer the command buffer
    void MigrateDescriptorEnvironment(ShaderExportStreamState* state, const PipelineState* pipeline, VkCommandBuffer commandBuffer);

    /// Process all segments within a queue
    /// \param queue the queue state
    void ProcessSegmentsNoQueueLock(ShaderExportQueueState* queue);

    /// Process a segment
    bool ProcessSegment(ShaderExportStreamSegment* segment);

    /// Free a segment
    void FreeSegmentNoQueueLock(ShaderExportQueueState* queue, ShaderExportStreamSegment* segment);

    /// Release a descriptor data segment
    void ReleaseDescriptorDataSegment(const DescriptorDataSegment& dataSegment);
    
private:
    DeviceDispatchTable* table;

    std::mutex mutex;

    BucketPoolAllocator<uint32_t> dynamicOffsetAllocator;

    ObjectPool<ShaderExportStreamState> streamStatePool;
    ObjectPool<ShaderExportStreamSegment> segmentPool;
    ObjectPool<ShaderExportQueueState> queuePool;

    /// All free descriptor segments
    std::vector<DescriptorDataSegmentEntry> freeDescriptorDataSegmentEntries;

    ComRef<DeviceAllocator> deviceAllocator{nullptr};
    ComRef<ShaderExportDescriptorAllocator> descriptorAllocator{nullptr};
    ComRef<ShaderExportStreamAllocator> streamAllocator{nullptr};
    ComRef<IBridge> bridge{nullptr};
};
