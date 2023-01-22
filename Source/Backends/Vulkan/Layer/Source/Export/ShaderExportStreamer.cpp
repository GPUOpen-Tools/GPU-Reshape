#include <Backends/Vulkan/Export/ShaderExportStreamer.h>
#include <Backends/Vulkan/Export/StreamState.h>
#include <Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/Vulkan/Export/ShaderExportStreamAllocator.h>
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/States/PipelineLayoutState.h>
#include <Backends/Vulkan/States/FenceState.h>
#include <Backends/Vulkan/States/QueueState.h>
#include <Backends/Vulkan/States/DescriptorSetState.h>
#include <Backends/Vulkan/Resource/DescriptorDataAppendAllocator.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTable.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Tables/InstanceDispatchTable.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStream.h>

// Common
#include <Common/Registry.h>
#include <Backends/Vulkan/Translation.h>

ShaderExportStreamer::ShaderExportStreamer(DeviceDispatchTable *table) : table(table), dynamicOffsetAllocator(table->allocators) {

}

bool ShaderExportStreamer::Install() {
    bridge = registry->Get<IBridge>();
    deviceAllocator = registry->Get<DeviceAllocator>();
    descriptorAllocator = registry->Get<ShaderExportDescriptorAllocator>();
    streamAllocator = registry->Get<ShaderExportStreamAllocator>();

    return true;
}

ShaderExportStreamer::~ShaderExportStreamer() {
    // Free all live segments
    for (ShaderExportQueueState* queue : queuePool) {
        for (ShaderExportStreamSegment* segment : queue->liveSegments) {
            FreeSegmentNoQueueLock(queue, segment);
        }
    }

    // Free all segments
    for (ShaderExportStreamSegment* segment : segmentPool) {
        streamAllocator->FreeSegment(segment->allocation);
    }
}

ShaderExportQueueState *ShaderExportStreamer::AllocateQueueState(QueueState* queue) {
    if (ShaderExportQueueState* queueState = queuePool.TryPop()) {
        return queueState;
    }

    // Create a new state
    auto* state = new (allocators) ShaderExportQueueState();
    state->queue = queue->object;

    // OK
    return state;
}

ShaderExportStreamState *ShaderExportStreamer::AllocateStreamState() {
    if (ShaderExportStreamState* streamState = streamStatePool.TryPop()) {
        return streamState;
    }

    // Create a new state
    auto* state = new (allocators) ShaderExportStreamState();

    // Create descriptor data allocator
    for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
        state->pipelineBindPoints[i].descriptorDataAllocator = new (allocators) DescriptorDataAppendAllocator(
            table,
            deviceAllocator,
            table->physicalDeviceProperties.limits.maxUniformBufferRange
        );
    }

    // OK
    return state;
}

void ShaderExportStreamer::Free(ShaderExportStreamState *state) {
    // Done
    streamStatePool.Push(state);
}

void ShaderExportStreamer::Free(ShaderExportQueueState *state) {
    // Done
    queuePool.Push(state);
}

ShaderExportStreamSegment *ShaderExportStreamer::AllocateSegment() {
    // Try existing allocation
    if (ShaderExportStreamSegment* segment = segmentPool.TryPop()) {
        return segment;
    }

    // Create new allocation
    auto* segment = new (allocators) ShaderExportStreamSegment();
    segment->allocation = streamAllocator->AllocateSegment();

    // OK
    return segment;
}

void ShaderExportStreamer::Enqueue(ShaderExportQueueState* queue, ShaderExportStreamSegment *segment, FenceState* fence) {
    ASSERT(!segment->fence, "Segment double submission");

    // Keep the fence alive
    fence->AddUser();

    // Assign fence and expected future state
    segment->fence = fence;
    segment->fenceNextCommitId = fence->GetNextCommitID();

    // OK
    queue->liveSegments.push_back(segment);
}

void ShaderExportStreamer::BeginCommandBuffer(ShaderExportStreamState* state, CommandBufferObject* commandBuffer) {
    for (ShaderExportPipelineBindState& bindState : state->pipelineBindPoints) {
        bindState.persistentDescriptorState.resize(table->physicalDeviceProperties.limits.maxBoundDescriptorSets);
        std::fill(bindState.persistentDescriptorState.begin(), bindState.persistentDescriptorState.end(), ShaderExportDescriptorState());

        // Reset state
        bindState.deviceDescriptorOverwriteMask = 0x0;
        bindState.pipeline = nullptr;
        bindState.isInstrumented = false;
    }

    // Clear push data
    state->persistentPushConstantData.resize(table->physicalDeviceProperties.limits.maxPushConstantsSize);
    std::fill(state->persistentPushConstantData.begin(), state->persistentPushConstantData.end(), 0u);

    // Initialize descriptor binders
    for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
        ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[i];

        // Recycle free data segments if available
        if (!freeDescriptorDataSegmentEntries.empty()) {
            bindState.descriptorDataAllocator->SetChunk(freeDescriptorDataSegmentEntries.back());
            freeDescriptorDataSegmentEntries.pop_back();
        }

        // Allocate a new descriptor set
        ShaderExportSegmentDescriptorAllocation allocation;
        allocation.info = descriptorAllocator->Allocate();
        state->segmentDescriptors.push_back(allocation);

        // Update all immutable descriptors, no descriptor data yet
        descriptorAllocator->UpdateImmutable(allocation.info, nullptr);

        // Set current for successive binds
        bindState.currentSegment = allocation;
    }
}

void ShaderExportStreamer::ResetCommandBuffer(ShaderExportStreamState *state, CommandBufferObject* commandBuffer) {
    for (ShaderExportPipelineBindState& bindState : state->pipelineBindPoints) {
        for (ShaderExportDescriptorState& descriptorState : bindState.persistentDescriptorState) {
            // Free all dynamic offsets
            if (descriptorState.dynamicOffsets) {
                std::lock_guard guard(mutex);
                dynamicOffsetAllocator.Free(descriptorState.dynamicOffsets);
            }
        }

        // Commit dangling data
        bindState.descriptorDataAllocator->Commit(nullptr);

        // Reset descriptor state
        bindState.persistentDescriptorState.resize(table->physicalDeviceProperties.limits.maxBoundDescriptorSets);
        std::fill(bindState.persistentDescriptorState.begin(), bindState.persistentDescriptorState.end(), ShaderExportDescriptorState());
        bindState.deviceDescriptorOverwriteMask = 0x0;
    }

    // Clear push data
    state->persistentPushConstantData.resize(table->physicalDeviceProperties.limits.maxPushConstantsSize);
    std::fill(state->persistentPushConstantData.begin(), state->persistentPushConstantData.end(), 0u);
}

void ShaderExportStreamer::EndCommandBuffer(ShaderExportStreamState* state, CommandBufferObject* commandBuffer) {
    for (ShaderExportPipelineBindState& bindState : state->pipelineBindPoints) {
        for (ShaderExportDescriptorState& descriptorState : bindState.persistentDescriptorState) {
            // Free all dynamic offsets
            if (descriptorState.dynamicOffsets) {
                std::lock_guard guard(mutex);
                dynamicOffsetAllocator.Free(descriptorState.dynamicOffsets);
            }
        }

        // Commit all host data to device
        bindState.descriptorDataAllocator->Commit(commandBuffer);
    }
}

void ShaderExportStreamer::BindPipeline(ShaderExportStreamState *state, const PipelineState *pipeline, VkPipeline object, bool instrumented, CommandBufferObject* commandBuffer) {
    // Get bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipeline->type)];

    // Restore the expected environment
    MigrateDescriptorEnvironment(state, pipeline, commandBuffer);

    // State tracking
    bindState.pipeline = pipeline;
    bindState.pipelineObject = object;
    bindState.isInstrumented = instrumented;

    // Ensure the shader export states are bound
    if (instrumented) {
        // Create initial descriptor segment
        bindState.descriptorDataAllocator->BeginSegment(pipeline->layout->boundUserDescriptorStates);

        // Set export set
        BindShaderExport(state, pipeline, commandBuffer);
    }
}

void ShaderExportStreamer::Process() {
    // ! Linear view locks
    for (QueueState* queueState : table->states_queue.GetLinear()) {
        ProcessSegmentsNoQueueLock(queueState->exportState);
    }
}

void ShaderExportStreamer::Process(ShaderExportQueueState* queueState) {
    std::lock_guard guard(table->states_queue.GetLock());
    ProcessSegmentsNoQueueLock(queueState);
}

void ShaderExportStreamer::Commit(ShaderExportStreamState *state, VkPipelineBindPoint bindPoint, CommandBufferObject *commandBuffer) {
    // Translate the bind point
    PipelineType pipelineType = Translate(bindPoint);

    // Get bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipelineType)];

    // If the allocator has rolled, a new segment is pending binding
    if (bindState.descriptorDataAllocator->HasRolled()) {
        // The underlying chunk may have changed, recreate the export data if needed
        if (!bindState.currentSegment.descriptorRollChunk || bindState.currentSegment.descriptorRollChunk != bindState.descriptorDataAllocator->GetSegmentBuffer()) {
            // Set current view
            bindState.currentSegment.descriptorRollChunk = bindState.descriptorDataAllocator->GetSegmentBuffer();

            // Allocate a new descriptor set
            ShaderExportSegmentDescriptorAllocation allocation;
            allocation.info = descriptorAllocator->Allocate();
            state->segmentDescriptors.push_back(allocation);

            // Update all immutable descriptors
            descriptorAllocator->UpdateImmutable(allocation.info, bindState.currentSegment.descriptorRollChunk);

            // Set current for successive binds
            bindState.currentSegment = allocation;
        }

        // Descriptor dynamic offset
        uint32_t dynamicOffset = static_cast<uint32_t>(bindState.descriptorDataAllocator->GetSegmentDynamicOffset());

        // Bind the export
        table->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
            commandBuffer->object,
            bindPoint, bindState.pipeline->layout->object,
            bindState.pipeline->layout->boundUserDescriptorStates, 1u, &bindState.currentSegment.info.set,
            1u, &dynamicOffset
        );
    }

    // Begin new segment
    bindState.descriptorDataAllocator->BeginSegment(bindState.pipeline->layout->boundUserDescriptorStates);
}

void ShaderExportStreamer::MigrateDescriptorEnvironment(ShaderExportStreamState *state, const PipelineState *pipeline, CommandBufferObject* commandBuffer) {
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipeline->type)];

    // Translate the bind point
    VkPipelineBindPoint vkBindPoint = Translate(pipeline->type);

    // Scan all overwritten descriptor sets
    unsigned long overwriteIndex;
    while (_BitScanForward(&overwriteIndex, bindState.deviceDescriptorOverwriteMask)) {
        // If the overwritten set is not part of the expected range, skip
        if (overwriteIndex >= pipeline->layout->boundUserDescriptorStates) {
            return;
        }

        // Get state
        ShaderExportDescriptorState& descriptorState = bindState.persistentDescriptorState[overwriteIndex];

        // May not have been used by the user, and the set may not be compatible anymore
        if (descriptorState.set && descriptorState.compatabilityHash == pipeline->layout->compatabilityHashes.at(overwriteIndex)) {
            // Debugging
#if TRACK_DESCRIPTOR_SETS
            commandBuffer->context.descriptorSets[static_cast<uint32_t>(pipeline->type)][overwriteIndex] = descriptorState.set;
#endif

            // Bind the expected set
            table->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
                commandBuffer->object,
                vkBindPoint, pipeline->layout->object,
                overwriteIndex, 1u, &descriptorState.set,
                descriptorState.dynamicOffsets.count, descriptorState.dynamicOffsets.data
            );
        } else {
            // Debugging
#if TRACK_DESCRIPTOR_SETS
            commandBuffer->context.descriptorSets[static_cast<uint32_t>(pipeline->type)][overwriteIndex] = nullptr;
#endif
        }

        // Push back to pool
        if (descriptorState.dynamicOffsets) {
            std::lock_guard guard(mutex);
            dynamicOffsetAllocator.Free(descriptorState.dynamicOffsets);
        }

        // Empty out (not really needed, but no need to micro-optimize confusing stuff)
        descriptorState = ShaderExportDescriptorState();

        // Next!
        bindState.deviceDescriptorOverwriteMask &= ~(1u << overwriteIndex);
    }
}

void ShaderExportStreamer::BindShaderExport(ShaderExportStreamState *state, PipelineType type, VkPipelineLayout layout, VkPipeline pipeline, uint32_t slot, CommandBufferObject *commandBuffer) {
    // Get the bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(type)];

    // Translate the bind point
    VkPipelineBindPoint vkBindPoint{};
    switch (type) {
        default:
            ASSERT(false, "Invalid pipeline type");
            break;
        case PipelineType::Graphics:
            vkBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            break;
        case PipelineType::Compute:
            vkBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            break;
    }

    // Descriptor dynamic offset
    uint32_t dynamicOffset = static_cast<uint32_t>(bindState.descriptorDataAllocator->GetSegmentDynamicOffset());

    // Bind the export
    table->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
        commandBuffer->object,
        vkBindPoint, layout,
        slot, 1u, &bindState.currentSegment.info.set,
        1u, &dynamicOffset
    );
}

void ShaderExportStreamer::BindShaderExport(ShaderExportStreamState *state, const PipelineState *pipeline, CommandBufferObject* commandBuffer) {
    // Get the bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipeline->type)];

    // Bind mask
    const uint32_t bindMask = 1u << pipeline->layout->boundUserDescriptorStates;

    // Already overwritten?
#if 0
    if (bindState.deviceDescriptorOverwriteMask & bindMask) {
        return;
    }
#endif

    // Debugging
#if TRACK_DESCRIPTOR_SETS
    commandBuffer->context.descriptorSets[static_cast<uint32_t>(pipeline->type)][pipeline->layout->boundUserDescriptorStates] = state->segmentDescriptorInfo.set;
#endif

    BindShaderExport(state, pipeline->type, pipeline->layout->object, pipeline->object, pipeline->layout->boundUserDescriptorStates, commandBuffer);

    // Mark as bound
    bindState.deviceDescriptorOverwriteMask |= bindMask;
}

void ShaderExportStreamer::MapSegment(ShaderExportStreamState *state, ShaderExportStreamSegment *segment) {
    for (const ShaderExportSegmentDescriptorAllocation &allocation: state->segmentDescriptors) {
        descriptorAllocator->Update(allocation.info, segment->allocation);
    }

    // Cleanup data segments
    for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
        ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[i];

        // Move descriptor data ownership to segment
        segment->descriptorDataSegments.push_back(bindState.descriptorDataAllocator->ReleaseSegment());
    }

    // Move ownership to the segment
    segment->segmentDescriptors.insert(segment->segmentDescriptors.end(), state->segmentDescriptors.begin(), state->segmentDescriptors.end());

    // Empty out
    state->segmentDescriptors.clear();
}

void ShaderExportStreamer::ProcessSegmentsNoQueueLock(ShaderExportQueueState* queue) {
    // TODO: Does not hold true for all queues
    auto it = queue->liveSegments.begin();

    // Segments are enqueued in order of completion
    for (; it != queue->liveSegments.end(); it++) {
        // If failed to process, none of the succeeding are ready
        if (!ProcessSegment(*it)) {
            break;
        }

        // Add back to pool
        FreeSegmentNoQueueLock(queue, *it);
    }

    // Remove dead segments
    queue->liveSegments.erase(queue->liveSegments.begin(), it);
}

bool ShaderExportStreamer::ProcessSegment(ShaderExportStreamSegment *segment) {
    // Ready?
    if (!segment->fence->IsCommitted(segment->fenceNextCommitId)) {
        return false;
    }

    // Output for messages
    IMessageStorage* output = bridge->GetOutput();

    // Map the counters
    const MirrorAllocation& counterMirror = segment->allocation->counter.allocation;
    auto* counters = static_cast<uint32_t*>(deviceAllocator->Map(counterMirror.host));

    // Process all streams
    for (size_t i = 0; i < segment->allocation->streams.size(); i++) {
        const ShaderExportStreamInfo& streamInfo = segment->allocation->streams[i];

        // Get the written counter
        uint32_t elementCount = counters[i];

        // Limit the counter by the physical size of the buffer (may exceed)
        elementCount = std::min(elementCount, static_cast<uint32_t>(streamInfo.byteSize / streamInfo.typeInfo.typeSize));

        // Map the stream
        auto* stream = static_cast<uint8_t*>(deviceAllocator->Map(streamInfo.allocation.host));

        // Size of the stream
        size_t size = elementCount * sizeof(uint32_t);

        // Copy into stream
        MessageStream messageStream;
        messageStream.SetSchema(streamInfo.typeInfo.messageSchema);
        messageStream.SetData(stream, size, static_cast<uint32_t>(size / streamInfo.typeInfo.typeSize));

        // Add output
        output->AddStream(messageStream);

        // Unmap
        deviceAllocator->Unmap(streamInfo.allocation.host);
    }

    // Unmap host
    deviceAllocator->Unmap(counterMirror.host);

    // Done!
    return true;
}

void ShaderExportStreamer::FreeSegmentNoQueueLock(ShaderExportQueueState* queue, ShaderExportStreamSegment *segment) {
    QueueState* queueState = table->states_queue.GetNoLock(queue->queue);

    // Move ownership to queue (don't release the reference count, queue owns it now)
    if (segment->fence->isImmediate) {
        queueState->pools_fences.Push(segment->fence);
    }

    // Release all descriptors
    for (const ShaderExportSegmentDescriptorAllocation& allocation : segment->segmentDescriptors) {
        descriptorAllocator->Free(allocation.info);
    }

    // Release all descriptor data
    for (const DescriptorDataSegment& dataSegment : segment->descriptorDataSegments) {
        if (dataSegment.entries.empty()) {
            continue;
        }

        // Free all re-chunked allocations
        for (size_t i = 0; i < dataSegment.entries.size() - 1; i++) {
            deviceAllocator->Free(dataSegment.entries[i].allocation);
        }

        // Mark the last, and largest, chunk as free
        freeDescriptorDataSegmentEntries.push_back(dataSegment.entries.back());
    }

    // Cleanup
    segment->segmentDescriptors.clear();
    segment->descriptorDataSegments.clear();

    // Remove fence reference
    segment->fence = nullptr;
    segment->fenceNextCommitId = 0;

    // Release command buffer
    queueState->PushCommandBuffer(segment->patchCommandBuffer);

    // Add back to pool
    segmentPool.Push(segment);
}

VkCommandBuffer ShaderExportStreamer::RecordPatchCommandBuffer(ShaderExportQueueState* state, ShaderExportStreamSegment* segment) {
    QueueState* queueState = table->states_queue.Get(state->queue);

    // Pop a new command buffer
    segment->patchCommandBuffer = queueState->PopCommandBuffer();

    // Begin info
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Attempt to begin
    if (table->next_vkBeginCommandBuffer(segment->patchCommandBuffer, &beginInfo) != VK_SUCCESS) {
        return nullptr;
    }

    // Counter to be copied
    const ShaderExportSegmentCounterInfo& counter = segment->allocation->counter;

    // Flush all queue work
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_READ_BIT;
    table->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
        segment->patchCommandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0x0,
        1, &barrier,
        0, nullptr,
        0, nullptr
    );

    // Copy the counter from device to host
    VkBufferCopy copy{};
    copy.size = sizeof(ShaderExportCounter) * segment->allocation->streams.size();
    table->commandBufferDispatchTable.next_vkCmdCopyBuffer(segment->patchCommandBuffer, counter.buffer, counter.bufferHost, 1u, &copy);

    // Flush all queue work
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    table->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
            segment->patchCommandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0x0,
            1, &barrier,
            0, nullptr,
            0, nullptr
    );

    // Clear device counters
    table->commandBufferDispatchTable.next_vkCmdFillBuffer(segment->patchCommandBuffer, counter.buffer, 0u, copy.size, 0x0);

    // Done
    table->next_vkEndCommandBuffer(segment->patchCommandBuffer);

    // OK
    return segment->patchCommandBuffer;
}

void ShaderExportStreamer::BindDescriptorSets(ShaderExportStreamState* state, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t start, uint32_t count, const VkDescriptorSet* sets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) {
    PipelineLayoutState* layoutState = table->states_pipelineLayout.Get(layout);

    // Get the bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(Translate(bindPoint))];

    // Current offset
    uint32_t dynamicOffset{0};

    // Handle each set
    for (uint32_t i = 0; i < count; i++) {
        uint32_t slot = start + i;

        // Get the state
        DescriptorSetState* persistentState = table->states_descriptorSet.Get(sets[i]);

        // Set the shader PRMT offset, roll the chunk if needed
        bindState.descriptorDataAllocator->SetOrAllocate(slot, layoutState->boundUserDescriptorStates, table->prmTable->GetSegmentShaderOffset(persistentState->segmentID));

        // Number of dynamic counts for this slot
        uint32_t slotDynamicCount = layoutState->descriptorDynamicOffsets.at(slot);

        // Clear the mask
        bindState.deviceDescriptorOverwriteMask &= ~(1u << slot);

        // Push back to pool if needed
        if (bindState.persistentDescriptorState[slot].dynamicOffsets) {
            std::lock_guard guard(mutex);
            dynamicOffsetAllocator.Free(bindState.persistentDescriptorState[slot].dynamicOffsets);
        }

        // Prepare state
        ShaderExportDescriptorState setState;
        setState.set = sets[i];
        setState.compatabilityHash = layoutState->compatabilityHashes[slot];

        // Allocate and copy dynamic offsets if needed
        if (slotDynamicCount) {
            // Scoped allocate
            {
                std::lock_guard guard(mutex);
                setState.dynamicOffsets = dynamicOffsetAllocator.Allocate(slotDynamicCount);
            }

            std::memcpy(setState.dynamicOffsets.data, pDynamicOffsets + dynamicOffset, sizeof(uint32_t) * slotDynamicCount);
        }

        // Set the set
        bindState.persistentDescriptorState[slot] = setState;

        // Apply offset
        dynamicOffset += slotDynamicCount;
    }
}
