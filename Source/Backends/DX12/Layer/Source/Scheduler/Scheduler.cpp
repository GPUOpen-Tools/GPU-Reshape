#include <Backends/DX12/Scheduler/Scheduler.h>
#include <Backends/DX12/States/CommandQueueState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/Command/UserCommandBuffer.h>

Scheduler::Scheduler(DeviceState *device) :
    queues(device->allocators),
    device(device) {

}

Scheduler::~Scheduler() {

}

static D3D12_COMMAND_LIST_TYPE GetType(Queue queue) {
    switch (queue) {
        default:
            ASSERT(false, "Invalid type");
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case Queue::Graphics:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case Queue::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case Queue::ExclusiveTransfer:
            return D3D12_COMMAND_LIST_TYPE_COPY;
    }
}

bool Scheduler::Install() {
    // Create all queues
    for (uint32_t i = 0; i < static_cast<uint32_t>(Queue::Count); i++) {
        QueueBucket& queue = queues.emplace_back(device->allocators);

        // Queue info
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = GetType(static_cast<Queue>(i));

        // Create exclusive queue
        if (FAILED(device->object->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue.queue)))) {
            return false;
        }
    }

    // OK
    return true;
}

void Scheduler::SyncPoint() {
    std::lock_guard guard(mutex);

    // Synchronize all queues
    for (QueueBucket& bucket : queues) {
        auto it = bucket.pendingSubmissions.begin();

        // Segments are enqueued in order of completion
        for (; it != bucket.pendingSubmissions.end(); it++) {
            // If failed to process, none of the succeeding are ready
            if (!it->fence->IsCommitted(it->fenceCommitID)) {
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

    // Temporary event
    HANDLE waitFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // Stall on all queues
    for (QueueBucket& bucket : queues) {
        for (Submission& submission : bucket.pendingSubmissions) {
            // Already done?
            if (submission.fence->IsCommitted(submission.fenceCommitID)) {
                continue;
            }

            // Wait for the pending submission
            submission.fence->fence->SetEventOnCompletion(submission.fenceCommitID, waitFenceEvent);
            WaitForSingleObject(waitFenceEvent, INFINITE);
        }
    }

    // Cleanup
    CloseHandle(waitFenceEvent);
}

void Scheduler::Schedule(Queue queue, const CommandBuffer &buffer) {
    std::lock_guard guard(mutex);

    // Get the queue
    QueueBucket& bucket = queues[static_cast<uint32_t>(queue)];

    // Get the next submission
    Submission submission = PopSubmission(queue);

    // Command generation
    {
        // Inform the streamer
        device->exportStreamer->BeginCommandList(submission.streamState, submission.commandList);

        // Commit all user commands
        CommitCommands(device, submission.commandList, buffer, submission.streamState);

        // Inform the streamer
        device->exportStreamer->CloseCommandList(submission.streamState);

        // Close
        submission.commandList->Close();
    }

    // Commit the fence index
    submission.fenceCommitID = submission.fence->CommitFence();

    // Submit the generated command list
    ID3D12CommandList* commandLists[] = {submission.commandList};
    bucket.queue->ExecuteCommandLists(1u, commandLists);
    bucket.queue->Signal(submission.fence->fence, submission.fenceCommitID);

    // Mark as pending
    bucket.pendingSubmissions.push_back(submission);
}

Scheduler::Submission Scheduler::PopSubmission(Queue queue) {
    Submission submission;

    // Get the queue
    QueueBucket& bucket = queues[static_cast<uint32_t>(queue)];

    // Translate type
    D3D12_COMMAND_LIST_TYPE type = GetType(queue);

    // Any free submissions?
    if (!bucket.freeSubmissions.empty()) {
        // Pop it!
        submission = bucket.freeSubmissions.back();
        bucket.freeSubmissions.pop_back();

        // Open / reset the command list
        submission.commandList->Reset(submission.allocator, nullptr);

        // OK
        return submission;
    }

    // Create allocator
    if (FAILED(device->object->CreateCommandAllocator(type, IID_PPV_ARGS(&submission.allocator)))) {
        return {};
    }

    // Attempt to create
    if (FAILED(device->object->CreateCommandList(
        0,
        type,
        submission.allocator,
        nullptr,
        IID_PPV_ARGS(&submission.commandList)
    ))) {
        return {};
    }

    // Create fence
    submission.fence = new (device->allocators) IncrementalFence();
    submission.fence->Install(device->object, bucket.queue);

    // Create streaming state
    submission.streamState = device->exportStreamer->AllocateStreamState();

    // OK
    return submission;
}
