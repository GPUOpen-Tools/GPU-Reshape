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

#include <Backends/DX12/Scheduler/Scheduler.h>
#include <Backends/DX12/States/CommandQueueState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/Command/UserCommandBuffer.h>
#include <Backend/Scheduler/SchedulerPrimitiveEvent.h>
#include <Backends/DX12/ShaderData/ShaderDataHost.h>

// Backend
#include <Backend/Scheduler/SchedulerTileMapping.h>

Scheduler::Scheduler(DeviceState *device) :
    queues(device->allocators),
    freePrimitives(device->allocators),
    primitives(device->allocators),
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

    // Get data host
    shaderDataHost = registry->Get<ShaderDataHost>();

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

            // Let the streamer recycle it
            device->exportStreamer->RecycleCommandList(it->streamState);

            // Add as free
            bucket.freeSubmissions.push_back(*it);
        }

        // Remove free submissions
        bucket.pendingSubmissions.erase(bucket.pendingSubmissions.begin(), it);
    }
}

ID3D12Fence * Scheduler::GetPrimitiveFence(SchedulerPrimitiveID pid) {
    return primitives[pid].fence;
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

void Scheduler::Schedule(Queue queue, const CommandBuffer &buffer, const SchedulerPrimitiveEvent* event) {
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

    // Submit the generated command list
    ID3D12CommandList* commandLists[] = {submission.commandList};
    bucket.queue->ExecuteCommandLists(1u, commandLists);

    // Commit the fence index (also signals the creation queue)
    submission.fenceCommitID = submission.fence->CommitFence();

    // Signal event if specified
    if (event) {
        bucket.queue->Signal(primitives[event->id].fence, event->value);
    }

    // Mark as pending
    bucket.pendingSubmissions.push_back(submission);
}

void Scheduler::MapTiles(Queue queue, ShaderDataID id, uint32_t count, const SchedulerTileMapping* mappings) {
    // Get allocation
    Allocation allocation = shaderDataHost->GetResourceAllocation(id);
    ASSERT(allocation.resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER, "Texture tile mappings not supported");

    // Get queue
    QueueBucket& bucket = queues[static_cast<uint32_t>(queue)];

    // Cached allocations
    TrivialStackVector<D3D12MA::Allocation*, 64> allocations;
    allocations.Resize(count);

    // All referenced heaps
    TrivialStackVector<ID3D12Heap*, 16> heaps;

    // Cache all allocations and determine their unique heaps
    for (uint32_t i = 0; i < count; i++) {
        const SchedulerTileMapping& mapping = mappings[i];

        // Cache allocation
        allocations[i] = shaderDataHost->GetMappingAllocation(mapping.mapping);

        // Heap already accounted for?
        if (std::find(heaps.begin(), heaps.end(), allocations[i]->GetHeap()) != heaps.end()) {
            continue;
        }

        // New unique heap
        heaps.Add(allocations[i]->GetHeap());
    }

    // All mapping properties
    TrivialStackVector<D3D12_TILED_RESOURCE_COORDINATE, 64> resourceCoordinates;
    TrivialStackVector<D3D12_TILE_REGION_SIZE, 64> resourceRegions;
    TrivialStackVector<uint32_t, 64> heapStartOffsets;
    TrivialStackVector<uint32_t, 64> heapTileCounts;

    // Assume worst case
    resourceCoordinates.Reserve(count);
    resourceRegions.Reserve(count);
    heapStartOffsets.Reserve(count);
    heapTileCounts.Reserve(count);

    // Batch on a per-heap basis
    for (uint32_t heapIndex = 0; heapIndex < heaps.Size(); heapIndex++) {
        ID3D12Heap* heap = heaps[heapIndex];

        // Append all allocations sharing this heap
        for (uint32_t i = 0; i < count; i++) {
            const SchedulerTileMapping& mapping = mappings[i];

            // Filter heap
            if (allocations[i]->GetHeap() != heap) {
                continue;
            }

            // Resource starting coordinate, in tiles
            resourceCoordinates.Add(D3D12_TILED_RESOURCE_COORDINATE {
                .X = mapping.tileOffset
            });

            // Resource tile region
            resourceRegions.Add(D3D12_TILE_REGION_SIZE {
                .NumTiles = mapping.tileCount
            });

            // Heap starting tile offset
            heapStartOffsets.Add( static_cast<uint32_t>(allocations[i]->GetOffset() / kShaderDataMappingTileWidth));

            // Heap tile count from offset
            heapTileCounts.Add(mapping.tileCount);
        }

        // Batch update the tile mappings
        bucket.queue->UpdateTileMappings(
            allocation.resource,
            static_cast<uint32_t>(resourceCoordinates.Size()),
            resourceCoordinates.Data(),
            resourceRegions.Data(),
            heap,
            static_cast<uint32_t>(heapStartOffsets.Size()),
            nullptr, 
            heapStartOffsets.Data(),
            heapTileCounts.Data(),
            D3D12_TILE_MAPPING_FLAG_NONE
        );

        // Cleanup for next iteration
        resourceCoordinates.Clear();
        resourceRegions.Clear();
        heapStartOffsets.Clear();
        heapTileCounts.Clear();
    }
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

    // Create the fence
    device->object->CreateFence(0u, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), reinterpret_cast<void**>(&entry.fence));

    // OK
    return pod;
}

void Scheduler::DestroyPrimitive(SchedulerPrimitiveID pid) {
    PrimitiveEntry& entry = primitives[pid];

    // Destroy the fence
    entry.fence->Release();
    entry.fence = nullptr;

    // Mark as free
    freePrimitives.push_back(pid);
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
