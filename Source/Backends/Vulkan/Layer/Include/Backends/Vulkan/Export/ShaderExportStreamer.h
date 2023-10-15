// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include <Common/Containers/TrivialStackVector.h>

// Std
#include <mutex>

// Forward declarations
class ShaderExportDescriptorAllocator;
class ShaderExportStreamAllocator;
class DeviceAllocator;
struct PhysicalResourceMappingTableQueueState;
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
    /// \param prmtState prmt state
    VkCommandBuffer RecordPreCommandBuffer(ShaderExportQueueState* queueState, ShaderExportStreamSegment* state, PhysicalResourceMappingTableQueueState* prmtState);

    /// Record a patch command buffer for submissions
    /// \param queueState the queue to record for
    /// \param state the segment state
    /// \return command buffer to be submitted
    VkCommandBuffer RecordPostCommandBuffer(ShaderExportQueueState* queueState, ShaderExportStreamSegment* state);

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

    /// Invoked during push descriptor binding
    /// \param state state to be committed to
    /// \param pipelineBindPoint the bind point
    /// \param layout the expected pipeline layout
    /// \param set the set index
    /// \param descriptorWriteCount number of writes
    /// \param pDescriptorWrites all writes
    /// \param commandBufferObject object committing from
    void PushDescriptorSetKHR(ShaderExportStreamState* state, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, VkCommandBuffer commandBufferObject);

    /// Invoked during push descriptor binding
    /// \param state state to be committed to
    /// \param descriptorUpdateTemplate the update template
    /// \param layout the expected pipeline layout
    /// \param set the set index
    /// \param pData update data
    /// \param commandBufferObject object committing from
    void PushDescriptorSetWithTemplateKHR(ShaderExportStreamState* state, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void* pData, VkCommandBuffer commandBufferObject);
    
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
    void ProcessSegmentsNoQueueLock(ShaderExportQueueState* queue, TrivialStackVector<CommandContextHandle, 32u>& completedHandles);

    /// Process a segment
    bool ProcessSegment(ShaderExportStreamSegment* segment, TrivialStackVector<CommandContextHandle, 32u>& completedHandles);

    /// Free a segment
    void FreeSegmentNoQueueLock(ShaderExportQueueState* queue, ShaderExportStreamSegment* segment);

    /// Release a descriptor data segment
    void ReleaseDescriptorDataSegment(const DescriptorDataSegment& dataSegment);
    
private:
    DeviceDispatchTable* table;

    /// Shared lock
    std::mutex mutex;

    /// Offset allocator
    BucketPoolAllocator<uint32_t> dynamicOffsetAllocator;

    /// Pooled objects
    ObjectPool<ShaderExportStreamState> streamStatePool;
    ObjectPool<ShaderExportStreamSegment> segmentPool;
    ObjectPool<ShaderExportQueueState> queuePool;

    /// All free descriptor segments
    std::vector<DescriptorDataSegmentEntry> freeDescriptorDataSegmentEntries;

    /// All components
    ComRef<DeviceAllocator> deviceAllocator{nullptr};
    ComRef<ShaderExportDescriptorAllocator> descriptorAllocator{nullptr};
    ComRef<ShaderExportStreamAllocator> streamAllocator{nullptr};
    ComRef<IBridge> bridge{nullptr};

    /// Does the device require push state tracking?
    bool requiresPushStateTracking{false};
};
