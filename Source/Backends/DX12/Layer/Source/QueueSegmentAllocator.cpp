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

#include <Backends/DX12/QueueSegmentAllocator.h>
#include <Backends/DX12/States/CommandQueueState.h>
#include <Backends/DX12/Export/ShaderExportStreamState.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/Command/UserCommandBuffer.h>

// Backend
#include <Backend/CommandContext.h>

QueueSegmentAllocator::QueueSegmentAllocator(DeviceState *device) : device(device) {
    
}

void QueueSegmentAllocator::ExecuteImmediate(CommandQueueState* queue, const CommandContext &context) {
    QueueSegment segment = PopSegment(queue);

    // Open the streamer state
    device->exportStreamer->BeginCommandList(segment.streamState, segment.immediate.commandList);
    {
        // Commit all commands
        CommitCommands(
            GetState(queue->parent),
            segment.immediate.commandList,
            context.buffer,
            segment.streamState
        );
    
        // Close the streamer state
        device->exportStreamer->CloseCommandList(segment.streamState);
    }
    
    // Done
    HRESULT hr = segment.immediate.commandList->Close();
    ASSERT(SUCCEEDED(hr), "Failed to close segment command list");

    // Submit!
    ID3D12CommandList* lists[] = {segment.immediate.commandList};
    queue->object->ExecuteCommandLists(1u, lists);

    // Commit fence forward
    segment.commitHead = queue->sharedFence->CommitFence();

    // Push for tracking
    PushSegment(segment);
}

QueueSegment QueueSegmentAllocator::PopSegment(CommandQueueState *queue) {
    std::lock_guard guard(mutex);

    // Check pending segments
    for (size_t i = 0; i < pendingSegments.size(); i++) {
        if (!pendingSegments[i].queue->sharedFence->IsCommitted(pendingSegments[i].commitHead)) {
            continue;
        }

        // Finished execution, pop it
        QueueSegment segment = pendingSegments[i];
        pendingSegments.erase(pendingSegments.begin() + i);

        // If switching queue, release and reacquire the command list
        if (queue != segment.queue) {
            segment.queue->PushCommandList(segment.immediate);
            segment.immediate = queue->PopCommandList();
            segment.queue = queue;
        }
            
        // Reopen
        HRESULT hr = segment.immediate.commandList->Reset(segment.immediate.allocator, nullptr);
        ASSERT(SUCCEEDED(hr), "Failed to reset command list");
        return segment;
    }

    // Create new segment
    QueueSegment segment;
    segment.queue = queue;
    segment.immediate = queue->PopCommandList();
    segment.streamState = device->exportStreamer->AllocateStreamState();
    return segment;
}

void QueueSegmentAllocator::PushSegment(const QueueSegment &segment) {
    std::lock_guard guard(mutex);
    pendingSegments.push_back(segment);
}
