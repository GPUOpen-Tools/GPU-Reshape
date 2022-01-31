#include <Backends/Vulkan/Export/ShaderExportStreamer.h>
#include <Backends/Vulkan/Export/StreamState.h>
#include <Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/Vulkan/Export/ShaderExportStreamAllocator.h>
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/States/PipelineLayoutState.h>
#include <Backends/Vulkan/States/FenceState.h>
#include <Backends/Vulkan/States/QueueState.h>
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

ShaderExportStreamer::ShaderExportStreamer(DeviceDispatchTable *table) : table(table) {

}

bool ShaderExportStreamer::Install() {
    bridge = registry->Get<IBridge>();

    deviceAllocator = registry->Get<DeviceAllocator>();
    descriptorAllocator = registry->Get<ShaderExportDescriptorAllocator>();
    streamAllocator = registry->Get<ShaderExportStreamAllocator>();

    return true;
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

    // Allocate a new descriptor set
    state->segmentDescriptorInfo = descriptorAllocator->Allocate();

    // OK
    return state;
}

void ShaderExportStreamer::Free(ShaderExportStreamState *state) {
    // Free the descriptor set
    descriptorAllocator->Free(state->segmentDescriptorInfo);

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

void ShaderExportStreamer::BeginCommandBuffer(ShaderExportStreamState* state, VkCommandBuffer commandBuffer) {
    for (ShaderExportPipelineBindState& bindState : state->pipelineBindPoints) {
        bindState.persistentDescriptorState.resize(table->physicalDeviceProperties.limits.maxBoundDescriptorSets);
        bindState.deviceDescriptorOverwriteMask = 0x0;
    }
}

void ShaderExportStreamer::BindPipeline(ShaderExportStreamState *state, const PipelineState *pipeline, bool instrumented, VkCommandBuffer commandBuffer) {
    // Restore the expected environment
    MigrateDescriptorEnvironment(state, pipeline, commandBuffer);

    // Ensure the shader export states are bound
    if (instrumented) {
        BindShaderExport(state, pipeline, commandBuffer);
    }
}

void ShaderExportStreamer::SyncPoint() {
    // ! Linear view locks
    for (QueueState* queueState : table->states_queue.GetLinear()) {
        ProcessSegmentsNoQueueLock(queueState->exportState);
    }
}

void ShaderExportStreamer::SyncPoint(ShaderExportQueueState* queueState) {
    std::lock_guard guard(table->states_queue.GetLock());
    ProcessSegmentsNoQueueLock(queueState);
}

void ShaderExportStreamer::MigrateDescriptorEnvironment(ShaderExportStreamState *state, const PipelineState *pipeline, VkCommandBuffer commandBuffer) {
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipeline->type)];

    // Scan all overwritten descriptor sets
    unsigned long overwriteIndex;
    while (_BitScanForward(&overwriteIndex, bindState.deviceDescriptorOverwriteMask)) {
        // If the overwritten set is not part of the expected range, skip
        if (overwriteIndex >= pipeline->layout->boundUserDescriptorStates) {
            return;
        }

        // Translate the bind point
        VkPipelineBindPoint vkBindPoint{};
        switch (pipeline->type) {
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

        // Bind the expected set
        table->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
            commandBuffer,
            vkBindPoint, pipeline->layout->object,
            overwriteIndex, 1u, &bindState.persistentDescriptorState[overwriteIndex],
            0u, nullptr
        );

        // Next!
        bindState.deviceDescriptorOverwriteMask &= ~(1u << overwriteIndex);
    }
}

void ShaderExportStreamer::BindShaderExport(ShaderExportStreamState *state, const PipelineState *pipeline, VkCommandBuffer commandBuffer) {
    // Get the bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipeline->type)];

    // Bind mask
    const uint32_t bindMask = 1u << pipeline->layout->boundUserDescriptorStates;

    // Already overwritten?
    if (bindState.deviceDescriptorOverwriteMask & bindMask) {
        return;
    }

    // Translate the bind point
    VkPipelineBindPoint vkBindPoint{};
    switch (pipeline->type) {
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

    // Bind the export
    table->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
        commandBuffer,
        vkBindPoint, pipeline->layout->object,
        pipeline->layout->boundUserDescriptorStates, 1u, &state->segmentDescriptorInfo.set,
        0u, nullptr
    );

    // Mark as bound
    bindState.deviceDescriptorOverwriteMask |= bindMask;
}

void ShaderExportStreamer::MapSegment(ShaderExportStreamState *state, ShaderExportStreamSegment *segment) {
    descriptorAllocator->Update(state->segmentDescriptorInfo, segment->allocation);
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
    const MirrorAllocation& counterMirror = segment->allocation->counter.allocation;;
    auto* counters = static_cast<uint32_t*>(deviceAllocator->Map(counterMirror.host));

    // Process all streams
    for (size_t i = 0; i < segment->allocation->streams.size(); i++) {
        const ShaderExportStreamInfo& streamInfo = segment->allocation->streams[i];

        // Get the written counter
        uint32_t elementCount = counters[i];

        // Map the stream
        auto* stream = static_cast<uint8_t*>(deviceAllocator->Map(streamInfo.allocation.host));

        // Size of the stream
        size_t size = elementCount * sizeof(uint32_t);

        // Copy into stream
        MessageStream messageStream;
        messageStream.SetSchema(streamInfo.typeInfo.messageSchema);
        messageStream.SetData(stream, size, size / streamInfo.typeInfo.typeSize);

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

    // Release fence usage
    segment->fence->ReleaseUser();

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

void ShaderExportStreamer::BindDescriptorSets(ShaderExportStreamState* state, VkPipelineBindPoint bindPoint, uint32_t start, uint32_t count, const VkDescriptorSet* sets) {
    // Get the bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(bindPoint)];

    // Handle each set
    for (uint32_t i = 0; i < count; i++) {
        uint32_t slot = start + i;

        // Clear the mask
        bindState.deviceDescriptorOverwriteMask &= ~(1u << i);

        // Set the set
        bindState.persistentDescriptorState[slot] = sets[i];
    }
}
