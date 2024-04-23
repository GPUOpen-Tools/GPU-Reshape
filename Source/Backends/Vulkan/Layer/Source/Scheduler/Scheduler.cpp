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

#include <Backends/Vulkan/Scheduler/Scheduler.h>
#include <Backends/Vulkan/States/QueueState.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Export/ShaderExportStreamer.h>
#include <Backends/Vulkan/Command/UserCommandBuffer.h>
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/ShaderData/ShaderDataHost.h>

// Backend
#include <Backend/Scheduler/SchedulerPrimitiveEvent.h>
#include <Backend/Scheduler/SchedulerTileMapping.h>

Scheduler::Scheduler(DeviceDispatchTable *table) :
    queues(table->allocators),
    freePrimitives(table->allocators),
    primitives(table->allocators),
    table(table) {

}

Scheduler::~Scheduler() {

}

bool Scheduler::Install() {
    // Install all queues
    InstallQueue(Queue::Graphics, table->preferredExclusiveGraphicsQueue);
    InstallQueue(Queue::Compute, table->preferredExclusiveComputeQueue);
    InstallQueue(Queue::ExclusiveTransfer, table->preferredExclusiveTransferQueue);

    // Get the allocator
    deviceAllocator = registry->Get<DeviceAllocator>();

    // OK
    return true;
}

void Scheduler::InstallQueue(Queue queue, ExclusiveQueue exclusiveQueue) {
    QueueBucket &bucket = queues.emplace_back(table->allocators);

    // Get underlying queue
    table->next_vkGetDeviceQueue(table->object, exclusiveQueue.familyIndex, exclusiveQueue.queueIndex, &bucket.queue);

    // Patch the dispatch table
    PatchInternalTable(bucket.queue, table->object);

    // Pool info
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = exclusiveQueue.familyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    // Attempt to create the pool
    if (table->next_vkCreateCommandPool(table->object, &poolInfo, nullptr, &bucket.pool) != VK_SUCCESS) {
        return;
    }
}

void Scheduler::SyncPoint() {
    std::lock_guard guard(mutex);

    // Synchronize all queues
    for (QueueBucket &bucket: queues) {
        auto it = bucket.pendingSubmissions.begin();

        // Segments are enqueued in order of completion
        for (; it != bucket.pendingSubmissions.end(); it++) {
            // If failed to process, none of the succeeding are ready
            if (table->next_vkGetFenceStatus(table->object, it->fence) == VK_SUCCESS) {
                break;
            }

            // Add as free
            bucket.freeSubmissions.push_back(*it);
        }

        // Remove free submissions
        bucket.pendingSubmissions.erase(bucket.pendingSubmissions.begin(), it);
    }
}

VkSemaphore Scheduler::GetPrimitiveSemaphore(SchedulerPrimitiveID pid) {
    return primitives[pid].semaphore;
}

void Scheduler::WaitForPending() {
    std::lock_guard guard(mutex);

    // Wait for all buckets
    for (QueueBucket &bucket: queues) {
        for (Submission &submission: bucket.pendingSubmissions) {
            // Already done?
            if (table->next_vkGetFenceStatus(table->object, submission.fence) == VK_SUCCESS) {
                continue;
            }

            // Wait for the fence
            table->next_vkWaitForFences(table->object, 1u, &submission.fence, true, UINT64_MAX);
        }
    }
}

void Scheduler::Schedule(Queue queue, const CommandBuffer &buffer, const SchedulerPrimitiveEvent* event) {
    std::lock_guard guard(mutex);

    // Get the queue
    QueueBucket &bucket = queues[static_cast<uint32_t>(queue)];

    // Get or create a submission
    Submission submission = PopSubmission(queue);

    // Reset the submission fence
    table->next_vkResetFences(table->object, 1u, &submission.fence);

    // Command generation
    {
        // Open the buffer
        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        table->next_vkBeginCommandBuffer(submission.commandBuffer, &beginInfo);

        // Inform the streamer
        table->exportStreamer->BeginCommandBuffer(submission.streamState, submission.commandBuffer);

        // Generate all user commands
        CommitCommands(table, submission.commandBuffer, buffer, submission.streamState);

        // Inform the streamer
        table->exportStreamer->EndCommandBuffer(submission.streamState, submission.commandBuffer);

        // Close the buffer
        table->commandBufferDispatchTable.next_vkEndCommandBuffer(submission.commandBuffer);
    }

    // All semaphores
    TrivialStackVector<VkSemaphoreSubmitInfo, 4> signalSemaphores;

    // Append signalling event if specified
    if (event) {
        VkSemaphoreSubmitInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        info.semaphore = primitives[event->id].semaphore;
        info.value = event->value;
        info.stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        signalSemaphores.Add(info);
    }

    // Command buffer info
    VkCommandBufferSubmitInfo commandSubmitInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    commandSubmitInfo.commandBuffer = submission.commandBuffer;

    // Submission info
    VkSubmitInfo2 submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submitInfo.commandBufferInfoCount = 1u;
    submitInfo.pCommandBufferInfos = &commandSubmitInfo;
    submitInfo.signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphores.Size());
    submitInfo.pSignalSemaphoreInfos = signalSemaphores.Data();
    
    // Select next
    PFN_vkQueueSubmit2 next = table->next_vkQueueSubmit2 ? table->next_vkQueueSubmit2 : table->next_vkQueueSubmit2KHR;

    // Submit
    {
        std::lock_guard queueGuard(table->states_queue.Get(bucket.queue)->mutex);
        next(bucket.queue, 1u, &submitInfo, submission.fence);
    }

    // Mark as pending
    bucket.pendingSubmissions.push_back(submission);
}

Scheduler::Submission Scheduler::PopSubmission(Queue queue) {
    Submission submission;

    // Get the bucket
    QueueBucket &bucket = queues[static_cast<uint32_t>(queue)];

    // Any free?
    if (!bucket.freeSubmissions.empty()) {
        // Pop it!
        submission = bucket.freeSubmissions.back();
        bucket.freeSubmissions.pop_back();

        // Reset the command buffer
        table->next_vkResetCommandBuffer(submission.commandBuffer, 0x0);

        // OK
        return submission;
    }

    // Allocation info
    VkCommandBufferAllocateInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    info.commandPool = bucket.pool;
    info.commandBufferCount = 1;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    // Attempt to allocate command buffer
    if (table->next_vkAllocateCommandBuffers(table->object, &info, &submission.commandBuffer) != VK_SUCCESS) {
        return {};
    }

    // Patch the dispatch table
    PatchInternalTable(submission.commandBuffer, table->object);

    // Fence info
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

    // Attempt to create fence
    if (table->next_vkCreateFence(table->object, &fenceCreateInfo, nullptr, &submission.fence) != VK_SUCCESS) {
        return {};
    }

    // Create streaming state
    submission.streamState = table->exportStreamer->AllocateStreamState();

    // OK
    return submission;
}

SchedulerPrimitiveID Scheduler::CreatePrimitive() {
    // Allocate index
    SchedulerPrimitiveID pod;
    if (freePrimitives.empty()) {
        pod = static_cast<uint32_t>(primitives.size());
        primitives.emplace_back();
    } else {
        pod = freePrimitives.back();
        freePrimitives.pop_back();
    }

    // Create allocation
    PrimitiveEntry& entry = primitives[pod];

    // Timeline info, default to 0
    VkSemaphoreTypeCreateInfo timelineInfo{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineInfo.initialValue = 0u;

    // Semaphore info
    VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    info.pNext = &timelineInfo;
    
    // Create the semaphore
    table->next_vkCreateSemaphore(
        table->object,
        &info,
        nullptr,
        &entry.semaphore
    );

    // OK
    return pod;
}

void Scheduler::DestroyPrimitive(SchedulerPrimitiveID pid) {
    PrimitiveEntry& entry = primitives[pid];

    // Destroy the semaphore
    table->next_vkDestroySemaphore(
        table->object,
        entry.semaphore,
        nullptr
    );
    entry.semaphore = VK_NULL_HANDLE;
}

void Scheduler::MapTiles(Queue queue, ShaderDataID id, uint32_t count, const SchedulerTileMapping* mappings) {
    // Get the bucket
    QueueBucket &bucket = queues[static_cast<uint32_t>(queue)];

    // Get the target buffer
    VkBuffer buffer = table->dataHost->GetResourceBuffer(id);

    // All memory bindings
    std::vector<VkSparseMemoryBind> memoryBinds(count);

    // Translate all bindings
    for (uint32_t i = 0; i < count; i++) {
        const SchedulerTileMapping& mapping = mappings[i];

        // Get the underlying allocation
        VmaAllocation     mappingAllocation = table->dataHost->GetMappingAllocation(mapping.mapping);
        VmaAllocationInfo mappingInfo       = deviceAllocator->GetAllocationInfo(mappingAllocation);

        // Assign offsets
        VkSparseMemoryBind& memoryInfo = memoryBinds[i] = {};
        memoryInfo.resourceOffset = mapping.tileOffset * kShaderDataMappingTileWidth;
        memoryInfo.size = mappings->tileCount * kShaderDataMappingTileWidth;
        memoryInfo.memory = mappingInfo.deviceMemory;
        memoryInfo.memoryOffset = mappingInfo.offset;
    }

    // Buffer binding info
    VkSparseBufferMemoryBindInfo bindInfo{};
    bindInfo.buffer = buffer;
    bindInfo.bindCount = static_cast<uint32_t>(memoryBinds.size());
    bindInfo.pBinds = memoryBinds.data();

    // Batch binding info
    VkBindSparseInfo info{VK_STRUCTURE_TYPE_BIND_SPARSE_INFO};
    info.bufferBindCount = 1u;
    info.pBufferBinds = &bindInfo;

    // Bind all tiles
    table->next_vkQueueBindSparse(
        bucket.queue,
        1u,
        &info,
        VK_NULL_HANDLE
    );
}
