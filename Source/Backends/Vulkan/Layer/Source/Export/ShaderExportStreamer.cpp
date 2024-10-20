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
#include <Backends/Vulkan/Resource/DescriptorData.h>
#include <Backends/Vulkan/Controllers/VersioningController.h>
#include <Backends/Vulkan/ShaderData/ShaderDataHost.h>
#include <Backends/Vulkan/Resource/PushDescriptorAppendAllocator.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTablePersistentVersion.h>

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

    // Check if push descriptor tracking is required
    for (const char* extension : table->enabledExtensions) {
        if (!std::strcmp(extension, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME)) {
            requiresPushStateTracking = true;
            break;
        }
    }

    // OK
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

    // Free all stream states
    for (ShaderExportStreamState* state : streamStatePool) {
        table->next_vkDestroyBuffer(table->object, state->constantShaderDataBuffer.buffer, nullptr);
        deviceAllocator->Free(state->constantShaderDataBuffer.allocation);
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
    std::lock_guard guard(mutex);

    // Existing?
    if (ShaderExportStreamState* streamState = streamStatePool.TryPop()) {
        return streamState;
    }

    // Create a new state
    auto* state = new (allocators) ShaderExportStreamState();

    // Create the constants data buffer
    state->constantShaderDataBuffer = table->dataHost->CreateConstantDataBuffer();

    // Create descriptor data allocator
    for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
        state->pipelineBindPoints[i].descriptorDataAllocator = new (allocators) DescriptorDataAppendAllocator(
            table,
            deviceAllocator,
            &state->renderPass,
            descriptorAllocator->GetBindingInfo().descriptorDataDescriptorLength
        );

        // Create push descriptor allocator if needed by the device
        if (requiresPushStateTracking) {
            state->pipelineBindPoints[i].pushDescriptorAppendAllocator = new (allocators) PushDescriptorAppendAllocator(
                table,
                state->pipelineBindPoints[i].descriptorDataAllocator
            );
        }
    }

    // OK
    return state;
}

void ShaderExportStreamer::Free(ShaderExportStreamState *state) {
    std::lock_guard guard(mutex);

    // Done
    streamStatePool.Push(state);
}

void ShaderExportStreamer::Free(ShaderExportQueueState *state) {
    std::lock_guard guard(mutex);

    // Done
    queuePool.Push(state);
}

ShaderExportStreamSegment *ShaderExportStreamer::AllocateSegment() {
    std::lock_guard guard(mutex);

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
    std::lock_guard queueGuard(table->states_queue.GetLock());
    queue->liveSegments.push_back(segment);
}

void ShaderExportStreamer::BeginCommandBuffer(ShaderExportStreamState* state, VkCommandBuffer commandBuffer) {
    // Recycle old data if needed
    if (state->pending) {
        ResetCommandBuffer(state);
    }

    // Serial
    std::lock_guard guard(mutex);

    for (ShaderExportPipelineBindState& bindState : state->pipelineBindPoints) {
        bindState.persistentDescriptorState.resize(table->physicalDeviceProperties.limits.maxBoundDescriptorSets);
        std::fill(bindState.persistentDescriptorState.begin(), bindState.persistentDescriptorState.end(), ShaderExportDescriptorState());

        // Reset state
        bindState.deviceDescriptorOverwriteMask = 0x0;
        bindState.pipeline = nullptr;
        bindState.pipelineObject = nullptr;
        bindState.isInstrumented = false;
    }

    // Reset render pass state
    state->renderPass.insideRenderPass = false;
    
    // Mark as pending
    state->pending = true;

    // Clear push data
    state->persistentPushConstantData.resize(table->physicalDeviceProperties.limits.maxPushConstantsSize);
    std::fill(state->persistentPushConstantData.begin(), state->persistentPushConstantData.end(), 0u);

    // Initialize descriptor binders
    for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
        ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[i];

        // Recycle free data segments if available
        if (!freeDescriptorDataSegmentEntries.empty()) {
            bindState.descriptorDataAllocator->SetChunk(commandBuffer, freeDescriptorDataSegmentEntries.back());
            freeDescriptorDataSegmentEntries.pop_back();
        } else {
            bindState.descriptorDataAllocator->ValidateReleased();
        }

        // Reset push allocator
        if (bindState.pushDescriptorAppendAllocator) {
            bindState.pushDescriptorAppendAllocator->Reset();
        }

        // Allocate a new descriptor set
        ShaderExportSegmentDescriptorAllocation allocation;
        allocation.info = descriptorAllocator->Allocate();
        state->segmentDescriptors.push_back(allocation);

        // Update all immutable descriptors, no descriptor data yet
        descriptorAllocator->UpdateImmutable(allocation.info, nullptr, state->constantShaderDataBuffer.buffer);

        // Set current for successive binds
        bindState.currentSegment = allocation;
    }
}

void ShaderExportStreamer::ResetCommandBuffer(ShaderExportStreamState *state) {
    std::lock_guard guard(mutex);

    // Release all bind points
    for (ShaderExportPipelineBindState& bindState : state->pipelineBindPoints) {
        for (ShaderExportDescriptorState& descriptorState : bindState.persistentDescriptorState) {
            // Free all dynamic offsets
            if (descriptorState.dynamicOffsets) {
                dynamicOffsetAllocator.Free(descriptorState.dynamicOffsets);
            }
        }

        // Commit dangling data
        bindState.descriptorDataAllocator->Commit();

        // Move descriptor data ownership to segment
        ReleaseDescriptorDataSegment(bindState.descriptorDataAllocator->ReleaseSegment());

        // Release all entries immediately
        if (bindState.pushDescriptorAppendAllocator) {
            bindState.pushDescriptorAppendAllocator->ReleaseSegment().ReleaseEntries();
        }

        // Reset descriptor state
        bindState.persistentDescriptorState.resize(table->physicalDeviceProperties.limits.maxBoundDescriptorSets);
        std::fill(bindState.persistentDescriptorState.begin(), bindState.persistentDescriptorState.end(), ShaderExportDescriptorState());
        bindState.deviceDescriptorOverwriteMask = 0x0;
    }

    // Release all descriptors
    for (const ShaderExportSegmentDescriptorAllocation& allocation : state->segmentDescriptors) {
        descriptorAllocator->Free(allocation.info);
    }
    
    // Clear push data
    state->persistentPushConstantData.resize(table->physicalDeviceProperties.limits.maxPushConstantsSize);
    std::fill(state->persistentPushConstantData.begin(), state->persistentPushConstantData.end(), 0u);

    // Cleanup
    state->segmentDescriptors.clear();
    
    // OK
    state->pending = false;
}

void ShaderExportStreamer::EndCommandBuffer(ShaderExportStreamState* state, VkCommandBuffer commandBuffer) {
    ASSERT(state->pending, "Recycling non-pending stream state");
    
    for (ShaderExportPipelineBindState& bindState : state->pipelineBindPoints) {
        for (ShaderExportDescriptorState& descriptorState : bindState.persistentDescriptorState) {
            // Free all dynamic offsets
            if (descriptorState.dynamicOffsets) {
                {
                    std::lock_guard guard(mutex);
                    dynamicOffsetAllocator.Free(descriptorState.dynamicOffsets);
                }

                // Cleanup
                descriptorState.dynamicOffsets = {};
            }
        }

        // Commit all host data
        bindState.descriptorDataAllocator->Commit();
    }
}

void ShaderExportStreamer::BindPipeline(ShaderExportStreamState *state, const PipelineState *pipeline, VkPipeline object, bool instrumented, VkCommandBuffer commandBuffer) {
    // Get bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipeline->type)];

    // Restore the expected environment
    MigrateDescriptorEnvironment(state, pipeline, commandBuffer);

    // Needs reconstruction of the descriptor segment?
    if (pipeline != bindState.pipeline || pipeline->layout->compatabilityHash != bindState.pipeline->layout->compatabilityHash) {
        // Begin new descriptor segment
        bindState.descriptorDataAllocator->BeginSegment(pipeline->layout->boundUserDescriptorStates * kDescriptorDataDWordCount, false);

        // Setup new segment
        for (size_t i = 0; i < pipeline->layout->compatabilityHashes.size(); i++) {
            const ShaderExportDescriptorState &descriptorState = bindState.persistentDescriptorState.at(i);

            // No persistent data? Mapped segment is null, just continue
            if (!descriptorState.set) {
                continue;
            }

            // Base dword offset for descriptor data
            const uint32_t descriptorDWordOffset = static_cast<uint32_t>(i) * kDescriptorDataDWordCount;

            // Mismatched compatability?
            if (pipeline->layout->compatabilityHashes[i] != descriptorState.compatabilityHash) {
                bindState.descriptorDataAllocator->Set(commandBuffer, descriptorDWordOffset + kDescriptorDataOffsetDWord, 0x0);
                continue;
            }
        
            // Get the state
            DescriptorSetState* persistentState = table->states_descriptorSet.Get(bindState.persistentDescriptorState[i].set);

            // Get the segment
            PhysicalResourceMappingTableSegment segment = table->prmTable->GetSegmentShader(persistentState->segmentID);

            // Set offset and length
            bindState.descriptorDataAllocator->Set(commandBuffer, descriptorDWordOffset + kDescriptorDataOffsetDWord, segment.offset);
            bindState.descriptorDataAllocator->Set(commandBuffer, descriptorDWordOffset + kDescriptorDataLengthDWord, segment.length);
        }

        // As the bindings have been (potentially) invalidated, we must roll the chunk
        bindState.descriptorDataAllocator->ConditionalRoll(commandBuffer);
    }

    // State tracking
    bindState.pipeline = pipeline;
    bindState.pipelineObject = object;
    bindState.isInstrumented = instrumented;

    // Ensure the shader export states are bound
    if (instrumented) {
        // Set export set
        BindShaderExport(state, pipeline, commandBuffer);
    }
}

void ShaderExportStreamer::Process() {
    // Released handles
    TrivialStackVector<CommandContextHandle, 32u> completedHandles;

    // Handle segments
    {
        // Maintain lock hierarchy, streamer -> queue
        std::lock_guard guard(mutex);
        
        // Process queues
        // ! Linear view locks
        for (QueueState* queueState : table->states_queue.GetLinear()) {
            ProcessSegmentsNoQueueLock(queueState->exportState, completedHandles);
        }
    }
    
    // Invoke proxies for all handles
    for (CommandContextHandle handle : completedHandles) {
        for (const FeatureHookTable &proxyTable: table->featureHookTables) {
            proxyTable.join.TryInvoke(handle);
        }
    }
}

void ShaderExportStreamer::Process(ShaderExportQueueState* queueState) {
    // Released handles
    TrivialStackVector<CommandContextHandle, 32u> completedHandles;

    // Handle segments
    {
        // Maintain lock hierarchy, streamer -> queue
        std::lock_guard guard(mutex);
        
        // Process queue
        std::lock_guard queueGuard(table->states_queue.GetLock());
        ProcessSegmentsNoQueueLock(queueState, completedHandles);
    }
    
    // Invoke proxies for all handles
    for (CommandContextHandle handle : completedHandles) {
        for (const FeatureHookTable &proxyTable: table->featureHookTables) {
            proxyTable.join.TryInvoke(handle);
        }
    }
}

void ShaderExportStreamer::Commit(ShaderExportStreamState *state, VkPipelineBindPoint bindPoint, VkCommandBuffer commandBuffer) {
    // Translate the bind point
    PipelineType pipelineType = Translate(bindPoint);

    // Get bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipelineType)];

    // Commit all push data
    if (bindState.pushDescriptorAppendAllocator) {
        bindState.pushDescriptorAppendAllocator->Commit(commandBuffer, bindPoint);
    }

    // If the allocator has rolled, a new segment is pending binding
    if (bindState.descriptorDataAllocator->HasRolled()) {
        // The underlying chunk may have changed, recreate the export data if needed
        if (!bindState.currentSegment.descriptorRollChunk || bindState.currentSegment.descriptorRollChunk != bindState.descriptorDataAllocator->GetSegmentBuffer()) {
            // Get current chunk
            VkBuffer chunkBuffer = bindState.descriptorDataAllocator->GetSegmentBuffer();
            
            // Allocate a new descriptor set
            ShaderExportSegmentDescriptorAllocation allocation;
            allocation.info = descriptorAllocator->Allocate();
            state->segmentDescriptors.push_back(allocation);

            // Update all immutable descriptors
            descriptorAllocator->UpdateImmutable(allocation.info, chunkBuffer, state->constantShaderDataBuffer.buffer);

            // Set current for successive binds
            bindState.currentSegment = allocation;
            
            // Set current view
            bindState.currentSegment.descriptorRollChunk = chunkBuffer;

#if PRMT_METHOD == PRMT_METHOD_UB_PC
            // Bind the new export
            table->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
                commandBuffer,
                bindPoint, bindState.pipeline->layout->object,
                bindState.pipeline->layout->boundUserDescriptorStates, 1u, &bindState.currentSegment.info.set,
                0u, nullptr
            );
#endif  // PRMT_METHOD == PRMT_METHOD_UB_PC
        }

#if PRMT_METHOD == PRMT_METHOD_UB_PC
        // Descriptor dynamic offset
        uint32_t dynamicOffset = static_cast<uint32_t>(bindState.descriptorDataAllocator->GetSegmentDynamicOffset());

        // Update offset
        table->commandBufferDispatchTable.next_vkCmdPushConstants(
            commandBuffer,
            bindState.pipeline->layout->object,
            VK_SHADER_STAGE_ALL,
            bindState.pipeline->layout->prmtPushConstantOffset,
            sizeof(uint32_t),
            &dynamicOffset
        );
#else // PRMT_METHOD == PRMT_METHOD_UB_PC
        // Descriptor dynamic offset
        uint32_t dynamicOffset = static_cast<uint32_t>(bindState.descriptorDataAllocator->GetSegmentDynamicOffset());

        // Bind the export
        table->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
            commandBuffer->object,
            bindPoint, bindState.pipeline->layout->object,
            bindState.pipeline->layout->boundUserDescriptorStates, 1u, &bindState.currentSegment.info.set,
            1u, &dynamicOffset
        );
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC
    }

    // Begin new segment
    bindState.descriptorDataAllocator->BeginSegment(bindState.pipeline->layout->boundUserDescriptorStates * kDescriptorDataDWordCount, true);
}

void ShaderExportStreamer::MigrateDescriptorEnvironment(ShaderExportStreamState *state, const PipelineState *pipeline, VkCommandBuffer commandBuffer) {
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipeline->type)];

    // Translate the bind point
    VkPipelineBindPoint vkBindPoint = Translate(pipeline->type);

    // Invalidate existing push entries
    if (bindState.pushDescriptorAppendAllocator) {
        bindState.pushDescriptorAppendAllocator->InvalidateOnCompatability(pipeline->layout);
    }
    
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
                commandBuffer,
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

void ShaderExportStreamer::BindShaderExport(ShaderExportStreamState *state, PipelineType type, VkPipelineLayout layout, VkPipeline pipeline, uint32_t prmtPushConstantOffset, uint32_t slot, VkCommandBuffer commandBuffer) {
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
        case PipelineType::Raytracing:
            vkBindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
            break;
    }

#if PRMT_METHOD == PRMT_METHOD_UB_PC
    // Descriptor dynamic offset
    uint32_t dynamicOffset = static_cast<uint32_t>(bindState.descriptorDataAllocator->GetSegmentDynamicOffset());

    // Bind the export
    table->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
        commandBuffer,
        vkBindPoint, layout,
        slot, 1u, &bindState.currentSegment.info.set,
        0u, nullptr
    );

    // Update offset
    table->commandBufferDispatchTable.next_vkCmdPushConstants(
        commandBuffer,
        layout,
        VK_SHADER_STAGE_ALL,
        prmtPushConstantOffset,
        sizeof(uint32_t),
        &dynamicOffset
    );
#else // PRMT_METHOD == PRMT_METHOD_UB_PC
    // Descriptor dynamic offset
    uint32_t dynamicOffset = static_cast<uint32_t>(bindState.descriptorDataAllocator->GetSegmentDynamicOffset());

    // Bind the export
    table->commandBufferDispatchTable.next_vkCmdBindDescriptorSets(
        commandBuffer->object,
        vkBindPoint, layout,
        slot, 1u, &bindState.currentSegment.info.set,
        1u, &dynamicOffset
    );
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC
}

void ShaderExportStreamer::BindShaderExport(ShaderExportStreamState *state, const PipelineState *pipeline, VkCommandBuffer commandBuffer) {
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

    BindShaderExport(
        state,
        pipeline->type,
        pipeline->layout->object,
        pipeline->object,
        pipeline->layout->prmtPushConstantOffset,
        pipeline->layout->boundUserDescriptorStates,
        commandBuffer
    );

    // Mark as bound
    bindState.deviceDescriptorOverwriteMask |= bindMask;
}

void ShaderExportStreamer::MapSegment(ShaderExportStreamState *state, ShaderExportStreamSegment *segment) {
    for (const ShaderExportSegmentDescriptorAllocation &allocation: state->segmentDescriptors) {
        descriptorAllocator->Update(allocation.info, segment->allocation, segment->prmtPersistentVersion);
    }

    // Add context handle
    ASSERT(state->commandContextHandle != kInvalidCommandContextHandle, "Unmapped command context handle");
    segment->commandContextHandles.push_back(state->commandContextHandle);
}

void ShaderExportStreamer::ProcessSegmentsNoQueueLock(ShaderExportQueueState* queue, TrivialStackVector<CommandContextHandle, 32u>& completedHandles) {
    // TODO: Does not hold true for all queues
    auto it = queue->liveSegments.begin();

    // Segments are enqueued in order of completion
    for (; it != queue->liveSegments.end(); it++) {
        // If failed to process, none of the succeeding are ready
        if (!ProcessSegment(*it, completedHandles)) {
            break;
        }

        // Add back to pool
        FreeSegmentNoQueueLock(queue, *it);
    }

    // Remove dead segments
    queue->liveSegments.erase(queue->liveSegments.begin(), it);
}

bool ShaderExportStreamer::ProcessSegment(ShaderExportStreamSegment *segment, TrivialStackVector<CommandContextHandle, 32u>& completedHandles) {
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
        messageStream.SetVersionID(segment->versionSegPoint.id);
        messageStream.SetData(stream, size, static_cast<uint32_t>(size / streamInfo.typeInfo.typeSize));

        // Add output
        output->AddStream(messageStream);

        // Unmap
        deviceAllocator->Unmap(streamInfo.allocation.host);
    }

    // Unmap host
    deviceAllocator->Unmap(counterMirror.host);

    // Inform the versioning controller of a collapse
    ASSERT(segment->versionSegPoint.id != UINT32_MAX, "Untracked versioning");
    table->versioningController->CollapseOnFork(segment->versionSegPoint);

    // Collect all handles
    for (CommandContextHandle handle : segment->commandContextHandles) {
        completedHandles.Add(handle);
    }

    // Done!
    return true;
}

void ShaderExportStreamer::FreeSegmentNoQueueLock(ShaderExportQueueState* queue, ShaderExportStreamSegment *segment) {
    // Get queue
    QueueState* queueState = table->states_queue.GetNoLock(queue->queue);

    // Move ownership to queue (don't release the reference count, queue owns it now)
    if (segment->fence->isImmediate) {
        queueState->pools_fences.Push(segment->fence);
    }

    // Cleanup
    segment->commandContextHandles.clear();

    // Remove fence reference
    segment->fence = nullptr;
    segment->fenceNextCommitId = 0;

    // Reset versioning
    segment->versionSegPoint = {};

    // Release command buffer
    queueState->PushCommandBuffer(segment->prePatchCommandBuffer);
    queueState->PushCommandBuffer(segment->postPatchCommandBuffer);

    // Release persistent version
    destroyRef(segment->prmtPersistentVersion, allocators);

    // Add back to pool
    segmentPool.Push(segment);
}

void ShaderExportStreamer::ReleaseDescriptorDataSegment(const DescriptorDataSegment &dataSegment) {
    if (dataSegment.entries.empty()) {
        return;
    }

    // Free all re-chunked allocations
    for (size_t i = 0; i < dataSegment.entries.size() - 1; i++) {
        deviceAllocator->Free(dataSegment.entries[i].allocation);
    }

    // Mark the last, and largest, chunk as free
    freeDescriptorDataSegmentEntries.push_back(dataSegment.entries.back());
}

VkCommandBuffer ShaderExportStreamer::RecordPreCommandBuffer(ShaderExportQueueState* state, ShaderExportStreamSegment* segment, PhysicalResourceMappingTableQueueState* prmtState) {
    std::lock_guard guard(mutex);

    // Get queue
    QueueState* queueState = table->states_queue.Get(state->queue);

    // Pop a new command buffer
    segment->prePatchCommandBuffer = queueState->PopCommandBuffer();

    // Begin info
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Attempt to begin
    if (table->next_vkBeginCommandBuffer(segment->prePatchCommandBuffer, &beginInfo) != VK_SUCCESS) {
        return nullptr;
    }

    // Has the counter data been initialized?
    //   Only required once per segment allocation, as the segments are recycled this usually
    //   only occurs during application startup.
    if (segment->allocation->pendingInitialization) {
        // Clear device counters
        VkBufferCopy copy{};
        copy.size = sizeof(ShaderExportCounter) * segment->allocation->streams.size();
        table->commandBufferDispatchTable.next_vkCmdFillBuffer(segment->prePatchCommandBuffer, segment->allocation->counter.buffer, 0u, copy.size, 0x0);

        // Flush barrier
        VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
        barrier.buffer = segment->allocation->counter.buffer;
        barrier.offset = 0;
        barrier.size = copy.size;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        // Flush the counter fill
        table->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
            segment->prePatchCommandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0x0,
            0, nullptr,
            1, &barrier,
            0, nullptr
        );

        // Mark as initialized
        segment->allocation->pendingInitialization = false;
    }

    // Update all PRM data
    segment->prmtPersistentVersion = table->prmTable->GetPersistentVersion(segment->prePatchCommandBuffer, prmtState);

    // OK
    return segment->prePatchCommandBuffer;
}

VkCommandBuffer ShaderExportStreamer::RecordPostCommandBuffer(ShaderExportQueueState* state, ShaderExportStreamSegment* segment) {
    std::lock_guard guard(mutex);

    // Get queue
    QueueState* queueState = table->states_queue.Get(state->queue);

    // Pop a new command buffer
    segment->postPatchCommandBuffer = queueState->PopCommandBuffer();

    // Begin info
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Attempt to begin
    if (table->next_vkBeginCommandBuffer(segment->postPatchCommandBuffer, &beginInfo) != VK_SUCCESS) {
        return nullptr;
    }

    // Counter to be copied
    const ShaderExportSegmentCounterInfo& counter = segment->allocation->counter;

    // Flush all queue work
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_READ_BIT;
    table->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
        segment->postPatchCommandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0x0,
        1, &barrier,
        0, nullptr,
        0, nullptr
    );

    // Copy the counter from device to host
    VkBufferCopy copy{};
    copy.size = sizeof(ShaderExportCounter) * segment->allocation->streams.size();
    table->commandBufferDispatchTable.next_vkCmdCopyBuffer(segment->postPatchCommandBuffer, counter.buffer, counter.bufferHost, 1u, &copy);

    // Flush all queue work
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_READ_BIT;
    table->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
            segment->postPatchCommandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0x0,
            1, &barrier,
            0, nullptr,
            0, nullptr
    );

    // Clear device counters
    table->commandBufferDispatchTable.next_vkCmdFillBuffer(segment->postPatchCommandBuffer, counter.buffer, 0u, copy.size, 0x0);

    // Flush all queue work
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
    table->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
            segment->postPatchCommandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0x0,
            1, &barrier,
            0, nullptr,
            0, nullptr
    );
    
    // OK
    return segment->postPatchCommandBuffer;
}

void ShaderExportStreamer::BindDescriptorSets(ShaderExportStreamState* state, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t start, uint32_t count, const VkDescriptorSet* sets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets, VkCommandBuffer commandBuffer) {
    PipelineLayoutState* layoutState = table->states_pipelineLayout.Get(layout);

    // Get the bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(Translate(bindPoint))];

    // Current offset
    uint32_t dynamicOffset{0};

    // Handle each set
    for (uint32_t i = 0; i < count; i++) {
        uint32_t slot = start + i;

        // With graphics pipeline libraries null sets are allowed
        if (!sets[i]) {
            ShaderExportDescriptorState setState;
            setState.set = nullptr;
            setState.compatabilityHash = 0;
            bindState.persistentDescriptorState[slot] = setState;
            continue;
        }

        // Get the state
        DescriptorSetState* persistentState = table->states_descriptorSet.Get(sets[i]);

        // Descriptor data
        const uint32_t descriptorDataDWordOffset = slot * kDescriptorDataDWordCount;
        const uint32_t descriptorDataDWordBound  = layoutState->boundUserDescriptorStates * kDescriptorDataDWordCount;

        // Set the shader PRMT offset, roll the chunk if needed (only initial set needs to roll)
        PhysicalResourceMappingTableSegment segment = table->prmTable->GetSegmentShader(persistentState->segmentID);
        bindState.descriptorDataAllocator->SetOrAllocate(commandBuffer, descriptorDataDWordOffset + kDescriptorDataLengthDWord, descriptorDataDWordBound, segment.length);
        bindState.descriptorDataAllocator->Set(commandBuffer, descriptorDataDWordOffset + kDescriptorDataOffsetDWord, segment.offset);

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

void ShaderExportStreamer::PushDescriptorSetKHR(ShaderExportStreamState* state, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, VkCommandBuffer commandBufferObject) {
    // Translate the bind point
    PipelineType pipelineType = Translate(pipelineBindPoint);

    // Get bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipelineType)];

    // Pass down to push allocator
    bindState.pushDescriptorAppendAllocator->PushDescriptorSetKHR(commandBufferObject, layout, set, descriptorWriteCount, pDescriptorWrites);
}

void ShaderExportStreamer::PushDescriptorSetWithTemplateKHR(ShaderExportStreamState *state, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void* pData, VkCommandBuffer commandBufferObject) {
    DescriptorUpdateTemplateState* updateTemplate = table->states_descriptorUpdateTemplateState.Get(descriptorUpdateTemplate);
    
    // Translate the bind point
    PipelineType pipelineType = Translate(updateTemplate->createInfo->pipelineBindPoint);

    // Get bind state
    ShaderExportPipelineBindState& bindState = state->pipelineBindPoints[static_cast<uint32_t>(pipelineType)];

    // Pass down to push allocator
    bindState.pushDescriptorAppendAllocator->PushDescriptorSetWithTemplateKHR(commandBufferObject, updateTemplate, layout, set, pData);
}
