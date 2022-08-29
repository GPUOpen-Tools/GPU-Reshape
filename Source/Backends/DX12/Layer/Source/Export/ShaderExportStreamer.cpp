#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/Export/ShaderExportStreamState.h>
#include <Backends/DX12/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/DX12/Export/ShaderExportStreamAllocator.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/CommandQueueState.h>
#include <Backends/DX12/States/FenceState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/RootSignatureState.h>
#include <Backends/DX12/States/DescriptorHeapState.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/IncrementalFence.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStream.h>

// Common
#include <Common/Registry.h>

ShaderExportStreamer::ShaderExportStreamer(DeviceState* device) : device(device), dynamicOffsetAllocator(device->allocators) {

}

bool ShaderExportStreamer::Install() {
    bridge = registry->Get<IBridge>();
    deviceAllocator = registry->Get<DeviceAllocator>();
    streamAllocator = registry->Get<ShaderExportStreamAllocator>();

    // Somewhat safe (TODO, cyclic allocator) bound
    constexpr uint32_t kSharedHeapBound = 64'000;

    // Heap info
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = kSharedHeapBound;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    // Create shared CPU heap
    if (FAILED(device->object->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&sharedCPUHeap)))) {
        return false;
    }

    // Set as GPU visible
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    // Create shared GPU heap
    if (FAILED(device->object->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&sharedGPUHeap)))) {
        return false;
    }

    // Create allocators
    sharedCPUHeapAllocator = new (device->allocators) ShaderExportDescriptorAllocator(device->object, sharedCPUHeap, kSharedHeapBound);
    sharedGPUHeapAllocator = new (device->allocators) ShaderExportDescriptorAllocator(device->object, sharedGPUHeap, kSharedHeapBound);

    // OK
    return true;
}

ShaderExportStreamer::~ShaderExportStreamer() {
    // Free all live segments
    for (CommandQueueState* state : device->states_Queues.GetLinear()) {
        if (state->exportState) {
            for (ShaderExportStreamSegment* segment : state->exportState->liveSegments) {
                FreeSegmentNoQueueLock(state, segment);
            }
        }
    }

    // Free all segments
    for (ShaderExportStreamSegment* segment : segmentPool) {
        streamAllocator->FreeSegment(segment->allocation);
    }
}

ShaderExportQueueState *ShaderExportStreamer::AllocateQueueState(ID3D12CommandQueue* queue) {
    if (ShaderExportQueueState* queueState = queuePool.TryPop()) {
        return queueState;
    }

    // Create a new state
    auto* state = new (allocators) ShaderExportQueueState();
    state->queue = queue;

    // OK
    return state;
}

ShaderExportStreamState *ShaderExportStreamer::AllocateStreamState() {
    if (ShaderExportStreamState* streamState = streamStatePool.TryPop()) {
        return streamState;
    }

    // Create a new state
    auto* state = new (allocators) ShaderExportStreamState();

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

void ShaderExportStreamer::Enqueue(CommandQueueState* queueState, ShaderExportStreamSegment *segment) {
    ASSERT(segment->fence == nullptr, "Segment double submission");

    // Assign expected future state
    segment->fence = queueState->sharedFence;
    segment->fenceNextCommitId = queueState->sharedFence->CommitFence();

    // OK
    queueState->exportState->liveSegments.push_back(segment);
}

void ShaderExportStreamer::BeginCommandBuffer(ShaderExportStreamState* state, CommandListState* commandList) {

}

void ShaderExportStreamer::SetDescriptorHeap(ShaderExportStreamState *state, DescriptorHeapState *heap) {
    if (heap->type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
        return;
    }

    ShaderExportSegmentDescriptorInfo descriptorInfo = heap->allocator->Allocate(2);

    // Set current for successive binds
    state->currentSegment = descriptorInfo;

    // Append for later mapping
    state->segmentDescriptors.push_back(descriptorInfo);
}

void ShaderExportStreamer::SetRootSignature(ShaderExportStreamState *state, const RootSignatureState *rootSignature) {
    state->rootSignature = rootSignature;
    state->pipelineSegmentMask = PipelineType::None;
}

void ShaderExportStreamer::BindPipeline(ShaderExportStreamState *state, const PipelineState *pipeline, bool instrumented, CommandListState* commandList) {
    // Ensure the shader export states are bound
    if (instrumented) {
        BindShaderExport(state, pipeline, commandList);
    }
}

void ShaderExportStreamer::Process() {
    // ! Linear view locks
    for (CommandQueueState* queueState : device->states_Queues.GetLinear()) {
        ProcessSegmentsNoQueueLock(queueState);
    }
}

void ShaderExportStreamer::Process(CommandQueueState* queueState) {
    std::lock_guard guard(device->states_Queues.GetLock());
    ProcessSegmentsNoQueueLock(queueState);
}

void ShaderExportStreamer::BindShaderExport(ShaderExportStreamState *state, const PipelineState *pipeline, CommandListState* commandList) {
    // Skip if already mapped
    if (state->pipelineSegmentMask & pipeline->type) {
        return;
    }

    switch (pipeline->type) {
        case PipelineType::None:
            ASSERT(false, "Invalid pipeline");
            break;
        case PipelineType::Graphics:
            commandList->object->SetGraphicsRootDescriptorTable(state->rootSignature->userRootCount, state->currentSegment.gpuHandle);
            break;
        case PipelineType::Compute:
            commandList->object->SetComputeRootDescriptorTable(state->rootSignature->userRootCount, state->currentSegment.gpuHandle);
            break;
    }

    // Mark as bound
    state->pipelineSegmentMask |= pipeline->type;
}

void ShaderExportStreamer::MapSegment(ShaderExportStreamState *state, ShaderExportStreamSegment *segment) {
    for (const ShaderExportSegmentDescriptorInfo& info : state->segmentDescriptors) {
        device->object->CreateUnorderedAccessView(
            segment->allocation->counter.allocation.device.resource, nullptr,
            &segment->allocation->counter.view,
            info.cpuHandle
        );

        for (uint64_t i = 0; i < segment->allocation->streams.size(); i++) {
            device->object->CreateUnorderedAccessView(
                segment->allocation->streams[i].allocation.device.resource, nullptr,
                &segment->allocation->streams[i].view,
                D3D12_CPU_DESCRIPTOR_HANDLE {.ptr = info.cpuHandle.ptr + sharedCPUHeapAllocator->GetAdvance() * (i + 1)}
            );
        }
    }
}

void ShaderExportStreamer::ProcessSegmentsNoQueueLock(CommandQueueState* queue) {
    // TODO: Does not hold true for all queues
    auto it = queue->exportState->liveSegments.begin();

    // Segments are enqueued in order of completion
    for (; it != queue->exportState->liveSegments.end(); it++) {
        // If failed to process, none of the succeeding are ready
        if (!ProcessSegment(*it)) {
            break;
        }

        // Add back to pool
        FreeSegmentNoQueueLock(queue, *it);
    }

    // Remove dead segments
    queue->exportState->liveSegments.erase(queue->exportState->liveSegments.begin(), it);
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

void ShaderExportStreamer::FreeSegmentNoQueueLock(CommandQueueState* queue, ShaderExportStreamSegment *segment) {
    // Remove fence reference
    segment->fence = nullptr;
    segment->fenceNextCommitId = 0;

    sharedCPUHeapAllocator->Free(segment->patchDeviceCPUDescriptor);
    sharedGPUHeapAllocator->Free(segment->patchDeviceGPUDescriptor);

    // Release command buffer
    queue->PushCommandList(segment->patchCommandBuffer);

    // Add back to pool
    segmentPool.Push(segment);
}

ID3D12GraphicsCommandList* ShaderExportStreamer::RecordPatchCommandBuffer(CommandQueueState* queueState, ShaderExportStreamSegment* segment) {
    segment->patchDeviceCPUDescriptor = sharedCPUHeapAllocator->Allocate(1);
    segment->patchDeviceGPUDescriptor = sharedGPUHeapAllocator->Allocate(1);

    // Counter to be copied
    const ShaderExportSegmentCounterInfo& counter = segment->allocation->counter;

    // Create CPU only descriptor
    device->object->CreateUnorderedAccessView(
        counter.allocation.device.resource, nullptr,
        &counter.view,
        segment->patchDeviceCPUDescriptor.cpuHandle
    );

    // Create GPU only descriptor
    device->object->CreateUnorderedAccessView(
        counter.allocation.device.resource, nullptr,
        &counter.view,
        segment->patchDeviceGPUDescriptor.cpuHandle
    );

    // Pop a new command buffer
    segment->patchCommandBuffer = queueState->PopCommandList();

    // Set heap
    segment->patchCommandBuffer->SetDescriptorHeaps(1u, &sharedGPUHeap);

    // Flush all pending work and transition to src
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = counter.allocation.device.resource;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    segment->patchCommandBuffer->ResourceBarrier(1u, &barrier);

    // Copy the counter from device to host
    segment->patchCommandBuffer->CopyBufferRegion(
        counter.allocation.host.resource, 0u,
        counter.allocation.device.resource, 0u,
        sizeof(ShaderExportCounter) * segment->allocation->streams.size()
    );

    // Wait for the transfer
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = counter.allocation.device.resource;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    segment->patchCommandBuffer->ResourceBarrier(1u, &barrier);

    uint32_t clearValue[4] = { 0x0, 0x0, 0x0, 0x0 };

    // Clear device counters
    segment->patchCommandBuffer->ClearUnorderedAccessViewUint(
        segment->patchDeviceGPUDescriptor.gpuHandle, segment->patchDeviceCPUDescriptor.cpuHandle,
        counter.allocation.device.resource,
        clearValue,
        0u,
        nullptr
    );

    // Done
    HRESULT hr = segment->patchCommandBuffer->Close();
    if (FAILED(hr)) {
        return nullptr;
    }

    // OK
    return segment->patchCommandBuffer;
}
