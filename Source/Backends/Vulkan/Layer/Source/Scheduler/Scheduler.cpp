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

Scheduler::Scheduler(DeviceDispatchTable *table) :
    queues(table->allocators),
    table(table) {

}

Scheduler::~Scheduler() {

}

bool Scheduler::Install() {
    // Install all queues
    InstallQueue(Queue::Graphics, table->preferredExclusiveGraphicsQueue);
    InstallQueue(Queue::Compute, table->preferredExclusiveComputeQueue);
    InstallQueue(Queue::ExclusiveTransfer, table->preferredExclusiveTransferQueue);

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

void Scheduler::Schedule(Queue queue, const CommandBuffer &buffer) {
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

    // Submission info
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1u;
    submitInfo.pCommandBuffers = &submission.commandBuffer;

    // Submit
    {
        std::lock_guard queueGuard(table->states_queue.Get(bucket.queue)->mutex);
        table->next_vkQueueSubmit(bucket.queue, 1u, &submitInfo, submission.fence);
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
