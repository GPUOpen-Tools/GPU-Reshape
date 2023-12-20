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

#include <Backends/DX12/Export/ShaderExportStreamer.h>
#include <Backends/DX12/Export/ShaderExportConstantAllocator.h>
#include <Backends/DX12/Export/ShaderExportStreamState.h>
#include <Backends/DX12/Export/ShaderExportFixedTwoSidedDescriptorAllocator.h>
#include <Backends/DX12/Export/ShaderExportStreamAllocator.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/CommandQueueState.h>
#include <Backends/DX12/States/FenceState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/RootSignatureState.h>
#include <Backends/DX12/States/DescriptorHeapState.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/Resource/PhysicalResourceMappingTable.h>
#include <Backends/DX12/Resource/DescriptorDataAppendAllocator.h>
#include <Backends/DX12/ShaderData/ShaderDataHost.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/IncrementalFence.h>
#include <Backends/DX12/Export/ShaderExportHost.h>
#include <Backends/DX12/Controllers/VersioningController.h>
#include <Backends/DX12/ShaderData/ShaderDataHost.h>
#include <Backends/DX12/States/RootSignaturePhysicalMapping.h>
#include <Backends/DX12/Resource/DescriptorResourceMapping.h>
#include <Backends/DX12/Resource/DescriptorData.h>
#include <Backends/DX12/Resource/ReservedConstantData.h>

// Bridge
#include <Bridge/IBridge.h>

// Backend
#include <Backend/IShaderExportHost.h>

// Message
#include <Message/IMessageStorage.h>
#include <Message/MessageStream.h>

// Common
#include <Common/Registry.h>

ShaderExportStreamer::ShaderExportStreamer(DeviceState *device)
    : device(device), dynamicOffsetAllocator(device->allocators),
      streamStatePool(device->allocators),
      segmentPool(device->allocators),
      queuePool(device->allocators),
      freeDescriptorDataSegmentEntries(allocators),
      freeConstantShaderDataBuffers(allocators),
      freeConstantAllocators(allocators) {

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

    // Create the shared layout
    descriptorLayout.Install(device);

    // Create allocators
    sharedCPUHeapAllocator = new (device->allocators, kAllocShaderExport) ShaderExportFixedTwoSidedDescriptorAllocator(device->object, sharedCPUHeap, 1u, descriptorLayout.Count(), 0u, kSharedHeapBound);
    sharedGPUHeapAllocator = new (device->allocators, kAllocShaderExport) ShaderExportFixedTwoSidedDescriptorAllocator(device->object, sharedGPUHeap, 1u, descriptorLayout.Count(), 0u, kSharedHeapBound);

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

    // Free all stream states
    for (ShaderExportStreamState* state : streamStatePool) {
        device->deviceAllocator->Free(state->constantShaderDataBuffer.allocation);
    }
}

ShaderExportQueueState *ShaderExportStreamer::AllocateQueueState(ID3D12CommandQueue* queue) {
    if (ShaderExportQueueState* queueState = queuePool.TryPop()) {
        return queueState;
    }

    // Create a new state
    auto* state = new (allocators, kAllocShaderExport) ShaderExportQueueState(allocators);
    state->queue = queue;

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
    auto* state = new (allocators, kAllocShaderExport) ShaderExportStreamState(allocators);

    // Setup bind states
    for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
        ShaderExportStreamBindState& bindState = state->bindStates[i];

        // Create descriptor data allocator
        bindState.descriptorDataAllocator = new (allocators, kAllocShaderExport) DescriptorDataAppendAllocator(allocators, deviceAllocator);
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
    auto* segment = new (allocators, kAllocShaderExport) ShaderExportStreamSegment(allocators);
    segment->allocation = streamAllocator->AllocateSegment();

    // OK
    return segment;
}

ShaderExportFixedTwoSidedDescriptorAllocator * ShaderExportStreamer::AllocateTwoSidedAllocator(ID3D12DescriptorHeap* heap, uint32_t virtualCount, uint32_t bound) {
#if DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
    uint32_t offset = 0;
#else // DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
    uint32_t offset = virtualCount;
#endif // DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
    
    return new (device->allocators, kAllocShaderExport) ShaderExportFixedTwoSidedDescriptorAllocator(
        device->object, 
        heap,
        1u,
        descriptorLayout.Count(),
        offset,
        bound
    );
}

void ShaderExportStreamer::Enqueue(CommandQueueState* queueState, ShaderExportStreamSegment *segment) {
    ASSERT(segment->fence == nullptr, "Segment double submission");

    // Assign expected future state
    segment->fence = queueState->sharedFence;
    segment->fenceNextCommitId = queueState->sharedFence->CommitFence();

    // OK
    queueState->exportState->liveSegments.push_back(segment);
}

void ShaderExportStreamer::BeginCommandList(ShaderExportStreamState* state, ID3D12GraphicsCommandList* commandList) {
    // Recycle old data if needed
    if (state->pending) {
        RecycleCommandList(state);
    }

    // Serial
    std::lock_guard guard(mutex);

    // Reset state
    state->resourceHeap = nullptr;
    state->samplerHeap = nullptr;
    state->pipelineSegmentMask = {};
    state->pipeline = nullptr;
    state->pipelineObject = nullptr;
    state->isInstrumented = false;
    state->pending = true;

    // Uses descriptors?
    if (commandList->GetType() != D3D12_COMMAND_LIST_TYPE_COPY) {
        // Set initial heap
        commandList->SetDescriptorHeaps(1u, &sharedGPUHeap);

        // Allocate initial segment from shared allocator
        ShaderExportSegmentDescriptorAllocation allocation;
        allocation.info = sharedGPUHeapAllocator->Allocate(descriptorLayout.Count());
        allocation.allocator = sharedGPUHeapAllocator;
        state->segmentDescriptors.push_back(allocation);

        // Assign constant data buffer
        if (freeConstantShaderDataBuffers.empty()) {
            state->constantShaderDataBuffer = device->shaderDataHost->CreateConstantDataBuffer();
        } else {
            state->constantShaderDataBuffer = freeConstantShaderDataBuffers.back();
            freeConstantShaderDataBuffers.pop_back();
        }

        // No user heap provided, map immutable to nothing
        MapImmutableDescriptors(allocation, nullptr, nullptr, state->constantShaderDataBuffer.view);

        // Set current for successive binds
        state->currentSegment = allocation.info;

        // Recycle free data segments if available
        for (uint32_t i = 0; !freeDescriptorDataSegmentEntries.empty() && i < static_cast<uint32_t>(PipelineType::Count); i++) {
            state->bindStates[i].descriptorDataAllocator->SetChunk(freeDescriptorDataSegmentEntries.back());
            freeDescriptorDataSegmentEntries.pop_back();
        }

        // Reset bind states
        for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
            ShaderExportStreamBindState& bindState = state->bindStates[i];

            // Free all allocations, but keep the blocks themselves alive
            bindState.rootConstantAllocator.ClearSubAllocations();

            // Clear all persistent parameters
            std::fill_n(bindState.persistentRootParameters, MaxRootSignatureDWord, ShaderExportRootParameterValue {});

            // Reset state
            bindState.rootSignature = nullptr;
#ifndef NDEBUG
            bindState.bindMask = 0;
#endif // NDEBUG
        }
    }

    // Pop constant allocator if available
    if (!freeConstantAllocators.empty()) {
        state->constantAllocator = freeConstantAllocators.back();
        freeConstantAllocators.pop_back();
    }
}

void ShaderExportStreamer::CloseCommandList(ShaderExportStreamState *state) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
        state->bindStates[i].descriptorDataAllocator->Commit();
    }
}

void ShaderExportStreamer::InvalidateHeapMappingsFor(ShaderExportStreamState *state, D3D12_DESCRIPTOR_HEAP_TYPE type) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
        ShaderExportStreamBindState &bindState = state->bindStates[i];

        // Check all
        for (uint32_t rootIndex = 0; bindState.rootSignature && rootIndex < bindState.rootSignature->logicalMapping.userRootCount; rootIndex++) {
            // Get the expected heap type
            D3D12_DESCRIPTOR_HEAP_TYPE heapType = bindState.rootSignature->logicalMapping.userRootHeapTypes[rootIndex];

            // If of same heap type, invalidate the parameter
            if (heapType == type) {
                bindState.persistentRootParameters[rootIndex].type = ShaderExportRootParameterValueType::None;
            }
        }

        // If there's a currently set root signature, all bindings have become invalidated
        if (bindState.rootSignature) {
            InvalidateDescriptorSlots(state, bindState, bindState.rootSignature, type);
        }
    }
}

void ShaderExportStreamer::InvalidateDescriptorSlots(ShaderExportStreamState* state, ShaderExportStreamBindState& bindState, const RootSignatureState* rootSignature, D3D12_DESCRIPTOR_HEAP_TYPE type) {
    // As the bindings have been invalidated, we must roll the chunk
    bindState.descriptorDataAllocator->ConditionalRoll();

    // Invalidate sampler bindings
    for (size_t i = 0; i < rootSignature->logicalMapping.userRootHeapTypes.size(); i++) {
        if (type != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES && rootSignature->logicalMapping.userRootHeapTypes[i] != type) {
            continue;
        }

        // Write invalidated slots
        if (rootSignature->logicalMapping.userRootHeapTypes[i] == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
            bindState.descriptorDataAllocator->Set(static_cast<uint32_t>(i), kDescriptorDataSamplerInvalidOffset);
        } else if (state->resourceHeap) {
            bindState.descriptorDataAllocator->Set(static_cast<uint32_t>(i), state->resourceHeap->GetVirtualRangeBound());
        }
    }
}

void ShaderExportStreamer::UpdateReservedHeapConstantData(ShaderExportStreamState *state, ID3D12GraphicsCommandList* commandList) {
    uint32_t dwords[static_cast<uint32_t>(ReservedConstantDataDWords::Prefix)];

    // Set the virtual bound of the heap, used for range and invalidation checking
    dwords[static_cast<uint32_t>(ReservedConstantDataDWords::ResourceHeapInvalidationBound)] = state->resourceHeap->GetVirtualRangeBound();

    // Write the data
    WriteReservedHeapConstantBuffer(state, dwords, static_cast<uint32_t>(ReservedConstantDataDWords::Prefix), commandList);
}

void ShaderExportStreamer::WriteReservedHeapConstantBuffer(ShaderExportStreamState *state, const uint32_t* dwords, uint32_t dwordCount, ID3D12GraphicsCommandList *commandList) {
    // Expected read state
    D3D12_RESOURCE_STATES readState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    // Graphics bit?
    if (commandList->GetType() != D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        readState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    // Shader Read -> Copy Dest
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = state->constantShaderDataBuffer.allocation.resource;
    barrier.Transition.StateBefore = readState;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    commandList->ResourceBarrier(1u, &barrier);

    // Allocate staging data
    ShaderExportConstantAllocation stagingAllocation = state->constantAllocator.Allocate(device->deviceAllocator, dwordCount * sizeof(uint32_t));

    // Update data
    std::memcpy(stagingAllocation.staging, dwords, sizeof(uint32_t) * dwordCount);

    // Copy from staging
    commandList->CopyBufferRegion(
        state->constantShaderDataBuffer.allocation.resource,
        0u,
        stagingAllocation.resource,
        stagingAllocation.offset,
        dwordCount * sizeof(uint32_t)
    );

    // Copy Dest -> Shader Read
    barrier.Transition.StateAfter = readState;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    commandList->ResourceBarrier(1u, &barrier);
}

void ShaderExportStreamer::SetDescriptorHeap(ShaderExportStreamState* state, DescriptorHeapState* heap, ID3D12GraphicsCommandList* commandList) {
    // Set heap
    // TODO: Just host this in an array, much, much cleaner...
    switch (heap->type) {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV: {
            // Same?
            if (heap == state->resourceHeap) {
                return;
            }
            
            state->resourceHeap = heap;
            break;
        }
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER: {
            // Same?
            if (heap == state->samplerHeap) {
                return;
            }
            
            state->samplerHeap = heap;
            break;
        }
        default: {
            return;
        }
    }

    // Invalidate the relevant persistent parameters
    InvalidateHeapMappingsFor(state, heap->type);
    
    // Add as referenced
    state->referencedHeaps.push_back(heap);

    // No need to recreate if not resource heap
    if (!state->resourceHeap) {
        return;
    }

    // Data race begone!
    std::lock_guard guard(mutex);

    // Allocate initial segment from shared allocator
    ShaderExportSegmentDescriptorAllocation allocation;
    allocation.info = state->resourceHeap->allocator->Allocate(descriptorLayout.Count());
    allocation.allocator = state->resourceHeap->allocator;
    state->segmentDescriptors.push_back(allocation);

    // Map immutable to current heap
    MapImmutableDescriptors(allocation, state->resourceHeap, state->samplerHeap, state->constantShaderDataBuffer.view);

    // Set current for successive binds
    state->currentSegment = allocation.info;

    // Changing descriptor set invalidates all bound information
    state->pipelineSegmentMask = {};

    // Bound instrumented pipeline?
    if (state->pipeline && state->isInstrumented) {
        ShaderExportStreamBindState &bindState = GetBindStateFromPipeline(state, state->pipeline);

        // If there's a valid root signature, bind the export states
        if (bindState.rootSignature) {
            BindShaderExport(state, state->pipeline, commandList);
        }
    }

    // Finally, update all reserved constant data
    UpdateReservedHeapConstantData(state, commandList);
}

void ShaderExportStreamer::SetComputeRootSignature(ShaderExportStreamState *state, const RootSignatureState *rootSignature, ID3D12GraphicsCommandList* commandList) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

    // Reset mask in case it's changed
    if (bindState.rootSignature) {
        state->pipelineSegmentMask &= ~PipelineTypeSet(PipelineType::Compute);
    }

    // Create initial descriptor segments
    if (bindState.rootSignature != rootSignature) {
        bindState.descriptorDataAllocator->BeginSegment(rootSignature->logicalMapping.userRootCount, false);
#ifndef NDEBUG
        bindState.bindMask = 0x0;
#endif // NDEBUG

        // Old bindings are invalidated
        for (ShaderExportRootParameterValue& persistent : bindState.persistentRootParameters) {
            persistent.type = ShaderExportRootParameterValueType::None;
        }

        // Invalidate all descriptor slots
        InvalidateDescriptorSlots(state, bindState, rootSignature, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
    }
    
    // Keep state
    bindState.rootSignature = rootSignature;

    // Ensure the shader export states are bound
    if (state->pipeline && state->pipeline->type == PipelineType::Compute && state->isInstrumented) {
        BindShaderExport(state, state->pipeline, commandList);
    }
}

void ShaderExportStreamer::SetGraphicsRootSignature(ShaderExportStreamState *state, const RootSignatureState *rootSignature, ID3D12GraphicsCommandList* commandList) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::GraphicsSlot)];

    // Reset mask in case it's changed
    if (bindState.rootSignature) {
        state->pipelineSegmentMask &= ~PipelineTypeSet(PipelineType::Graphics);
    }

    // Create initial descriptor segments
    if (bindState.rootSignature != rootSignature) {
        bindState.descriptorDataAllocator->BeginSegment(rootSignature->logicalMapping.userRootCount, false);
#ifndef NDEBUG
        bindState.bindMask = 0x0;
#endif // NDEBUG

        // Old bindings are invalidated
        for (ShaderExportRootParameterValue& persistent : bindState.persistentRootParameters) {
            persistent.type = ShaderExportRootParameterValueType::None;
        }

        // Invalidate all descriptor slots
        InvalidateDescriptorSlots(state, bindState, rootSignature, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
    }
    
    // Keep state
    bindState.rootSignature = rootSignature;

    // Ensure the shader export states are bound
    if (state->pipeline && state->pipeline->type == PipelineType::Graphics && state->isInstrumented) {
        BindShaderExport(state, state->pipeline, commandList);
    }
}

void ShaderExportStreamer::CommitCompute(ShaderExportStreamState* state, ID3D12GraphicsCommandList* commandList) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

#ifndef NDEBUG
    bindState.descriptorDataAllocator->ValidateAgainst(bindState.bindMask);
#endif // NDEBUG

    // If the allocator has rolled, a new segment is pending binding
    if (bindState.descriptorDataAllocator->HasRolled()) {
        commandList->SetComputeRootConstantBufferView(bindState.rootSignature->logicalMapping.userRootCount + 1, bindState.descriptorDataAllocator->GetSegmentVirtualAddress());
    }

    // Begin new segment
    bindState.descriptorDataAllocator->BeginSegment(bindState.rootSignature->logicalMapping.userRootCount, true);
}

void ShaderExportStreamer::CommitGraphics(ShaderExportStreamState* state, ID3D12GraphicsCommandList* commandList) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::GraphicsSlot)];

#ifndef NDEBUG
    bindState.descriptorDataAllocator->ValidateAgainst(bindState.bindMask);
#endif // NDEBUG
    
    // If the allocator has rolled, a new segment is pending binding
    if (bindState.descriptorDataAllocator->HasRolled()) {
        commandList->SetGraphicsRootConstantBufferView(bindState.rootSignature->logicalMapping.userRootCount + 1, bindState.descriptorDataAllocator->GetSegmentVirtualAddress());
    }

    // Begin new segment
    bindState.descriptorDataAllocator->BeginSegment(bindState.rootSignature->logicalMapping.userRootCount, true);
}

ShaderExportStreamBindState& ShaderExportStreamer::GetBindStateFromPipeline(ShaderExportStreamState *state, const PipelineState* pipeline) {
    // Get slot
    PipelineType slot;
    switch (pipeline->type) {
        default:
            ASSERT(false, "Invalid pipeline");
            slot = PipelineType::None;
            break;
        case PipelineType::Graphics:
            slot = PipelineType::GraphicsSlot;
            break;
        case PipelineType::Compute:
            slot = PipelineType::ComputeSlot;
            break;
    }

    // Get bind state from slot
    return state->bindStates[static_cast<uint32_t>(slot)];
}

void ShaderExportStreamer::BindPipeline(ShaderExportStreamState *state, const PipelineState *pipeline, ID3D12PipelineState* pipelineObject, bool instrumented, ID3D12GraphicsCommandList* commandList) {
    // Get bind state from slot
    ShaderExportStreamBindState& bindState = GetBindStateFromPipeline(state, pipeline);

    // Set state
    state->pipeline = pipeline;
    state->pipelineObject = pipelineObject;
    state->isInstrumented = instrumented;

    // Invalidated root signature?
    if (bindState.rootSignature != pipeline->signature) {
        state->pipelineSegmentMask &= ~PipelineTypeSet(pipeline->type);

        // Invalidate the signature itself if the hash changed
        if (bindState.rootSignature && bindState.rootSignature->physicalMapping->signatureHash != pipeline->signature->physicalMapping->signatureHash) {
            bindState.rootSignature = nullptr;
        }
    }

    // Ensure the shader export states are bound
    if (bindState.rootSignature && instrumented) {
        BindShaderExport(state, pipeline, commandList);
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
        for (CommandQueueState* queueState : device->states_Queues.GetLinear()) {
            ProcessSegmentsNoQueueLock(queueState, completedHandles);
        }
    }
    
    // Invoke proxies for all handles
    for (CommandContextHandle handle : completedHandles) {
        for (const FeatureHookTable &proxyTable: device->featureHookTables) {
            proxyTable.join.TryInvoke(handle);
        }
    }
}

void ShaderExportStreamer::Process(CommandQueueState* queueState) {
    // Released handles
    TrivialStackVector<CommandContextHandle, 32u> completedHandles;

    // Handle segments
    {
        // Maintain lock hierarchy, streamer -> queue
        std::lock_guard guard(mutex);
        
        // Process queue
        std::lock_guard queueGuard(device->states_Queues.GetLock());
        ProcessSegmentsNoQueueLock(queueState, completedHandles);
    }
    
    // Invoke proxies for all handles
    for (CommandContextHandle handle : completedHandles) {
        for (const FeatureHookTable &proxyTable: device->featureHookTables) {
            proxyTable.join.TryInvoke(handle);
        }
    }
}

void ShaderExportStreamer::RecycleCommandList(ShaderExportStreamState *state) {
    std::lock_guard guard(mutex);
    ASSERT(state->pending, "Recycling non-pending stream state");

    // Move descriptor data ownership to segment
    for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
        FreeDescriptorDataSegment(state->bindStates[i].descriptorDataAllocator->ReleaseSegment());
    }

    // Move constant ownership to the segment
    freeConstantShaderDataBuffers.push_back(state->constantShaderDataBuffer);

    // Move constant allocator to the segment
    if (!state->constantAllocator.staging.empty()) {
        FreeConstantAllocator(state->constantAllocator);
    }

    // Move ownership to the segment
    for (const ShaderExportSegmentDescriptorAllocation& segmentDescriptor : state->segmentDescriptors) {
        segmentDescriptor.allocator->Free(segmentDescriptor.info);
    }

    // Cleanup
    state->constantShaderDataBuffer = {};
    state->segmentDescriptors.clear();
    state->referencedHeaps.clear();

    // OK
    state->pending = false;
}

void ShaderExportStreamer::BindShaderExport(ShaderExportStreamState *state, uint32_t slot, PipelineType type, ID3D12GraphicsCommandList *commandList) {
    switch (type) {
        default:
            ASSERT(false, "Invalid pipeline");
            break;
        case PipelineType::Graphics:
            commandList->SetGraphicsRootDescriptorTable(slot, state->currentSegment.gpuHandle);
            break;
        case PipelineType::Compute:
            commandList->SetComputeRootDescriptorTable(slot, state->currentSegment.gpuHandle);
            break;
    }
}

void ShaderExportStreamer::BindShaderExport(ShaderExportStreamState *state, const PipelineState *pipeline, ID3D12GraphicsCommandList* commandList) {
    // Skip if already mapped
    if (state->pipelineSegmentMask & pipeline->type) {
        return;
    }

    // Get binds
    ShaderExportStreamBindState& bindState = GetBindStateFromPipeline(state, pipeline);

    // Set on bind state
    BindShaderExport(state, bindState.rootSignature->logicalMapping.userRootCount, pipeline->type, commandList);

    // Mark as bound
    state->pipelineSegmentMask |= pipeline->type;
}

void ShaderExportStreamer::MapImmutableDescriptors(const ShaderExportSegmentDescriptorAllocation& descriptors, DescriptorHeapState* resourceHeap, DescriptorHeapState* samplerHeap, const D3D12_CONSTANT_BUFFER_VIEW_DESC& constantsChunk) {
    // Null descriptor for missing heaps
    D3D12_SHADER_RESOURCE_VIEW_DESC nullView{};
    nullView.Format = DXGI_FORMAT_R32_UINT;
    nullView.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    nullView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    nullView.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    nullView.Buffer.NumElements = 1u;

    // Has relevant heap?
    if (resourceHeap) {
        ASSERT(resourceHeap->type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Mismatch heap type");
        
        // Create view to PRMT buffer
        device->object->CreateShaderResourceView(
            resourceHeap->prmTable->GetResource(),
            &resourceHeap->prmTable->GetView(),
            descriptorLayout.GetResourcePRMT(descriptors.info.cpuHandle)
        );
    } else {
        device->object->CreateShaderResourceView(nullptr, &nullView, descriptorLayout.GetResourcePRMT(descriptors.info.cpuHandle));
    }

    // Has relevant heap?
    if (samplerHeap) {
        ASSERT(samplerHeap->type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Mismatch heap type");
        
        // Create view to PRMT buffer
        device->object->CreateShaderResourceView(
            samplerHeap->prmTable->GetResource(),
            &samplerHeap->prmTable->GetView(),
            descriptorLayout.GetSamplerPRMT(descriptors.info.cpuHandle)
        );
    } else {
        device->object->CreateShaderResourceView(nullptr, &nullView, descriptorLayout.GetSamplerPRMT(descriptors.info.cpuHandle));
    }

    // Create constants CBV
    device->object->CreateConstantBufferView(&constantsChunk, descriptorLayout.GetShaderConstants(descriptors.info.cpuHandle));

    // Create views to shader resources
    device->shaderDataHost->CreateDescriptors(descriptorLayout.GetShaderData(descriptors.info.cpuHandle, 0), sharedCPUHeapAllocator->GetAdvance());
}

void ShaderExportStreamer::MapSegment(ShaderExportStreamState *state, ShaderExportStreamSegment *segment) {
    // Map the command state to shared segment
    for (const ShaderExportSegmentDescriptorAllocation& allocation : state->segmentDescriptors) {
        // Update the segment counters
        device->object->CreateUnorderedAccessView(
            segment->allocation->counter.allocation.device.resource, nullptr,
            &segment->allocation->counter.view,
            descriptorLayout.GetExportCounter(allocation.info.cpuHandle)
        );

        // Update the segment streams
        for (uint64_t i = 0; i < segment->allocation->streams.size(); i++) {
            device->object->CreateUnorderedAccessView(
                segment->allocation->streams[i].allocation.device.resource, nullptr,
                &segment->allocation->streams[i].view,
                descriptorLayout.GetExportStream(allocation.info.cpuHandle, static_cast<uint32_t>(i))
            );
        }
    }

    // Move descriptor data ownership to segment
    for (uint32_t i = 0; i < static_cast<uint32_t>(PipelineType::Count); i++) {
        segment->descriptorDataSegments.push_back(state->bindStates[i].descriptorDataAllocator->ReleaseSegment());
    }

    // Move constant ownership to the segment
    segment->constantShaderDataBuffers.push_back(state->constantShaderDataBuffer);
    state->constantShaderDataBuffer = {};

    // Move constant allocator to the segment
    if (!state->constantAllocator.staging.empty()) {
        segment->constantAllocator.push_back(state->constantAllocator);
        state->constantAllocator = {};
    }

    // Add context handle
    ASSERT(state->commandContextHandle != kInvalidCommandContextHandle, "Unmapped command context handle");
    segment->commandContextHandles.push_back(state->commandContextHandle);

    // Move ownership to the segment
    segment->segmentDescriptors.insert(segment->segmentDescriptors.end(), state->segmentDescriptors.begin(), state->segmentDescriptors.end());
    segment->referencedHeaps.insert(segment->referencedHeaps.end(), state->referencedHeaps.begin(), state->referencedHeaps.end());

    // Empty out
    state->segmentDescriptors.clear();
    state->referencedHeaps.clear();

    // Data has been migrated
    state->pending = false;
}

void ShaderExportStreamer::SetComputeRootDescriptorTable(ShaderExportStreamState* state, UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

    // Store persistent
    bindState.persistentRootParameters[rootParameterIndex] = ShaderExportRootParameterValue::Descriptor(baseDescriptor);
#ifndef NDEBUG
    bindState.bindMask |= (1ull << rootParameterIndex);
#endif // NDEBUG

    // Referenced heap
    DescriptorHeapState* heap{nullptr};

    // Determine the referenced heap
    // Note that it's faster to check the cached heap bounds instead of doing a binary lookup
    if (state->resourceHeap && state->resourceHeap->IsInBounds(baseDescriptor)) {
        heap = state->resourceHeap;
    } else if (state->samplerHeap && state->samplerHeap->IsInBounds(baseDescriptor)) {
        heap = state->samplerHeap;
    } else {
        ASSERT(false, "Binding descriptor table of unknown or unbound heap");
        return;
    }

    // Get offset
    uint64_t offset = baseDescriptor.ptr - heap->gpuDescriptorBase.ptr;
    ASSERT(offset % heap->stride == 0, "Mismatched heap stride");

    // Set the root PRMT offset
    bindState.descriptorDataAllocator->Set(rootParameterIndex, static_cast<uint32_t>(offset / heap->stride));
#ifndef NDEBUG
    ASSERT((bindState.descriptorDataAllocator->GetBindMask() & bindState.bindMask) == bindState.bindMask, "Lost descriptor data");
#endif // NDEBUG
}

void ShaderExportStreamer::SetGraphicsRootDescriptorTable(ShaderExportStreamState* state, UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::GraphicsSlot)];

    // Store persistent
    bindState.persistentRootParameters[rootParameterIndex] = ShaderExportRootParameterValue::Descriptor(baseDescriptor);
#ifndef NDEBUG
    bindState.bindMask |= (1ull << rootParameterIndex);
#endif // NDEBUG

    // Referenced heap
    DescriptorHeapState* heap{nullptr};

    // Determine the referenced heap
    // Note that it's faster to check the cached heap bounds instead of doing a binary lookup
    if (state->resourceHeap && state->resourceHeap->IsInBounds(baseDescriptor)) {
        heap = state->resourceHeap;
    } else if (state->samplerHeap && state->samplerHeap->IsInBounds(baseDescriptor)) {
        heap = state->samplerHeap;
    } else {
        ASSERT(false, "Binding descriptor table of unknown or unbound heap");
        return;
    }
    
    // Get offset
    uint64_t offset = baseDescriptor.ptr - heap->gpuDescriptorBase.ptr;
    ASSERT(offset % heap->stride == 0, "Mismatched heap stride");

    // Set the root PRMT offset
    bindState.descriptorDataAllocator->Set(rootParameterIndex, static_cast<uint32_t>(offset / heap->stride));
#ifndef NDEBUG
    ASSERT((bindState.descriptorDataAllocator->GetBindMask() & bindState.bindMask) == bindState.bindMask, "Lost descriptor data");
#endif // NDEBUG
}

void ShaderExportStreamer::SetComputeRootShaderResourceView(ShaderExportStreamState* state, UINT rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

    // Store persistent
    bindState.persistentRootParameters[rootParameterIndex] = ShaderExportRootParameterValue::VirtualAddress(ShaderExportRootParameterValueType::SRV, bufferLocation);
#ifndef NDEBUG
    bindState.bindMask |= (1ull << rootParameterIndex);
#endif // NDEBUG

    // Set the root PRMT offset
    ResourceState* resourceState = device->virtualAddressTable.Find(bufferLocation);
    bindState.descriptorDataAllocator->Set(rootParameterIndex, resourceState ? resourceState->virtualMapping.opaque : GetUndefinedVirtualResourceMapping().opaque);
#ifndef NDEBUG
    ASSERT((bindState.descriptorDataAllocator->GetBindMask() & bindState.bindMask) == bindState.bindMask, "Lost descriptor data");
#endif // NDEBUG
}

void ShaderExportStreamer::SetGraphicsRootShaderResourceView(ShaderExportStreamState* state, UINT rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::GraphicsSlot)];

    // Store persistent
    bindState.persistentRootParameters[rootParameterIndex] = ShaderExportRootParameterValue::VirtualAddress(ShaderExportRootParameterValueType::SRV, bufferLocation);
#ifndef NDEBUG
    bindState.bindMask |= (1ull << rootParameterIndex);
#endif // NDEBUG

    // Set the root PRMT offset
    ResourceState* resourceState = device->virtualAddressTable.Find(bufferLocation);
    bindState.descriptorDataAllocator->Set(rootParameterIndex, resourceState ? resourceState->virtualMapping.opaque : GetUndefinedVirtualResourceMapping().opaque);
#ifndef NDEBUG
    ASSERT((bindState.descriptorDataAllocator->GetBindMask() & bindState.bindMask) == bindState.bindMask, "Lost descriptor data");
#endif // NDEBUG
}

void ShaderExportStreamer::SetComputeRootUnorderedAccessView(ShaderExportStreamState* state, UINT rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

    // Store persistent
    bindState.persistentRootParameters[rootParameterIndex] = ShaderExportRootParameterValue::VirtualAddress(ShaderExportRootParameterValueType::UAV, bufferLocation);
#ifndef NDEBUG
    bindState.bindMask |= (1ull << rootParameterIndex);
#endif // NDEBUG

    // Set the root PRMT offset
    ResourceState* resourceState = device->virtualAddressTable.Find(bufferLocation);
    bindState.descriptorDataAllocator->Set(rootParameterIndex, resourceState ? resourceState->virtualMapping.opaque : GetUndefinedVirtualResourceMapping().opaque);
#ifndef NDEBUG
    ASSERT((bindState.descriptorDataAllocator->GetBindMask() & bindState.bindMask) == bindState.bindMask, "Lost descriptor data");
#endif // NDEBUG
}

void ShaderExportStreamer::SetGraphicsRootUnorderedAccessView(ShaderExportStreamState* state, UINT rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::GraphicsSlot)];

    // Store persistent
    bindState.persistentRootParameters[rootParameterIndex] = ShaderExportRootParameterValue::VirtualAddress(ShaderExportRootParameterValueType::UAV, bufferLocation);
#ifndef NDEBUG
    bindState.bindMask |= (1ull << rootParameterIndex);
#endif // NDEBUG

    // Set the root PRMT offset
    ResourceState* resourceState = device->virtualAddressTable.Find(bufferLocation);
    bindState.descriptorDataAllocator->Set(rootParameterIndex, resourceState ? resourceState->virtualMapping.opaque : GetUndefinedVirtualResourceMapping().opaque);
#ifndef NDEBUG
    ASSERT((bindState.descriptorDataAllocator->GetBindMask() & bindState.bindMask) == bindState.bindMask, "Lost descriptor data");
#endif // NDEBUG
}
    
void ShaderExportStreamer::SetComputeRootConstantBufferView(ShaderExportStreamState* state, UINT rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

    // Store persistent
    bindState.persistentRootParameters[rootParameterIndex] = ShaderExportRootParameterValue::VirtualAddress(ShaderExportRootParameterValueType::CBV, bufferLocation);
#ifndef NDEBUG
    bindState.bindMask |= (1ull << rootParameterIndex);
#endif // NDEBUG

    // Set the root PRMT offset
    ResourceState* resourceState = device->virtualAddressTable.Find(bufferLocation);
    bindState.descriptorDataAllocator->Set(rootParameterIndex, resourceState ? resourceState->virtualMapping.opaque : GetUndefinedVirtualResourceMapping().opaque);
#ifndef NDEBUG
    ASSERT((bindState.descriptorDataAllocator->GetBindMask() & bindState.bindMask) == bindState.bindMask, "Lost descriptor data");
#endif // NDEBUG
}

void ShaderExportStreamer::SetGraphicsRootConstantBufferView(ShaderExportStreamState* state, UINT rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::GraphicsSlot)];

    // Store persistent
    bindState.persistentRootParameters[rootParameterIndex] = ShaderExportRootParameterValue::VirtualAddress(ShaderExportRootParameterValueType::CBV, bufferLocation);
#ifndef NDEBUG
    bindState.bindMask |= (1ull << rootParameterIndex);
#endif // NDEBUG

    // Set the root PRMT offset
    ResourceState* resourceState = device->virtualAddressTable.Find(bufferLocation);
    bindState.descriptorDataAllocator->Set(rootParameterIndex, resourceState ? resourceState->virtualMapping.opaque : GetUndefinedVirtualResourceMapping().opaque);
#ifndef NDEBUG
    ASSERT((bindState.descriptorDataAllocator->GetBindMask() & bindState.bindMask) == bindState.bindMask, "Lost descriptor data");
#endif // NDEBUG
}

void ShaderExportStreamer::SetGraphicsRootConstants(ShaderExportStreamState* state, UINT rootParameterIndex, const void* data, uint64_t size, uint64_t offset) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::GraphicsSlot)];

    // Root parameter
    ShaderExportRootParameterValue& value = bindState.persistentRootParameters[rootParameterIndex];

    // Constant root parameters are stateful, must be checked and initialized carefully
    if (value.type != ShaderExportRootParameterValueType::Constant) {
        value.payload.constant.data = nullptr;
        value.payload.constant.dataByteCount = 0;
        value.type = ShaderExportRootParameterValueType::Constant;
    }

    // End offset
    uint64_t end = size + offset;

    // Reallocate if out of space
    if (value.payload.constant.dataByteCount < end) {
        value.payload.constant.dataByteCount = static_cast<uint32_t>(end);
        value.payload.constant.data = bindState.rootConstantAllocator.AllocateArray<uint8_t>(value.payload.constant.dataByteCount);
    }

    // Copy data
    std::memcpy(static_cast<uint8_t*>(value.payload.constant.data) + offset, data, size);
}

void ShaderExportStreamer::SetComputeRootConstants(ShaderExportStreamState* state, UINT rootParameterIndex, const void* data, uint64_t size, uint64_t offset) {
    // Bind state
    ShaderExportStreamBindState& bindState = state->bindStates[static_cast<uint32_t>(PipelineType::ComputeSlot)];

    // Root parameter
    ShaderExportRootParameterValue& value = bindState.persistentRootParameters[rootParameterIndex];

    // Constant root parameters are stateful, must be checked and initialized carefully
    if (value.type != ShaderExportRootParameterValueType::Constant) {
        value.payload.constant.data = nullptr;
        value.payload.constant.dataByteCount = 0;
        value.type = ShaderExportRootParameterValueType::Constant;
    }

    // End offset
    uint64_t end = size + offset;

    // Reallocate if out of space
    if (value.payload.constant.dataByteCount < end) {
        value.payload.constant.dataByteCount = static_cast<uint32_t>(end);
        value.payload.constant.data = bindState.rootConstantAllocator.AllocateArray<uint8_t>(value.payload.constant.dataByteCount);
    }

    // Copy data
    std::memcpy(static_cast<uint8_t*>(value.payload.constant.data) + offset, data, size);
}

void ShaderExportStreamer::ProcessSegmentsNoQueueLock(CommandQueueState* queue, TrivialStackVector<CommandContextHandle, 32u>& completedHandles) {
    // TODO: Does not hold true for all queues
    auto it = queue->exportState->liveSegments.begin();

    // Segments are enqueued in order of completion
    for (; it != queue->exportState->liveSegments.end(); it++) {
        // If failed to process, none of the succeeding are ready
        if (!ProcessSegment(*it, completedHandles)) {
            break;
        }

        // Add back to pool
        FreeSegmentNoQueueLock(queue, *it);
    }

    // Remove dead segments
    queue->exportState->liveSegments.erase(queue->exportState->liveSegments.begin(), it);
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
        size_t size = std::min<uint64_t>(elementCount * sizeof(uint32_t), streamInfo.allocation.host.allocation->GetSize());

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
    device->versioningController->CollapseOnFork(segment->versionSegPoint);

    // Collect all handles
    for (CommandContextHandle handle : segment->commandContextHandles) {
        completedHandles.Add(handle);
    }

    // Done!
    return true;
}

void ShaderExportStreamer::FreeConstantAllocator(ShaderExportConstantAllocator& allocator) {
    // Cleanup the staging data
    if (!allocator.staging.empty()) {
        // Free all staging allocations except the last
        for (size_t i = 0; i < allocator.staging.size() - 1u; i++) {
            device->deviceAllocator->Free(allocator.staging[i].allocation);
        }

        // Remove all but the last
        allocator.staging.erase(allocator.staging.begin(), allocator.staging.begin() + (allocator.staging.size() - 1u));

        // Reset head counter
        allocator.staging.back().head = 0;
    }

    // Add to free pool
    freeConstantAllocators.push_back(allocator);
}

void ShaderExportStreamer::FreeDescriptorDataSegment(const DescriptorDataSegment &dataSegment) {
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

void ShaderExportStreamer::FreeSegmentNoQueueLock(CommandQueueState* queue, ShaderExportStreamSegment *segment) {
    // Remove fence reference
    segment->fence = nullptr;
    segment->fenceNextCommitId = 0;

    // Reset versioning
    segment->versionSegPoint = {};

    // Release all descriptors to their respective owners
    for (const ShaderExportSegmentDescriptorAllocation& allocation : segment->segmentDescriptors) {
        allocation.allocator->Free(allocation.info);
    }

    // Release all descriptor data
    for (const DescriptorDataSegment& dataSegment : segment->descriptorDataSegments) {
        FreeDescriptorDataSegment(dataSegment);
    }

    // Release all constant data buffers
    for (ConstantShaderDataBuffer& constantData : segment->constantShaderDataBuffers) {
        freeConstantShaderDataBuffers.push_back(constantData);
    }

    // Release all constant allocators
    for (ShaderExportConstantAllocator& constantAllocator : segment->constantAllocator) {
        FreeConstantAllocator(constantAllocator);
    }

    // Release patch descriptors
    sharedCPUHeapAllocator->Free(segment->patchDeviceCPUDescriptor);
    sharedGPUHeapAllocator->Free(segment->patchDeviceGPUDescriptor);

    // Release command list
    queue->PushCommandList(segment->immediatePrePatch);
    queue->PushCommandList(segment->immediatePostPatch);

    // Cleanup
    segment->segmentDescriptors.clear();
    segment->referencedHeaps.clear();
    segment->descriptorDataSegments.clear();
    segment->immediatePrePatch = {};
    segment->immediatePostPatch = {};
    segment->patchDeviceCPUDescriptor = {};
    segment->patchDeviceGPUDescriptor = {};
    segment->commandContextHandles.clear();
    segment->constantShaderDataBuffers.clear();
    segment->constantAllocator.clear();

    // Add back to pool
    segmentPool.Push(segment);
}

ID3D12GraphicsCommandList* ShaderExportStreamer::RecordPreCommandList(CommandQueueState* queueState, ShaderExportStreamSegment* segment) {
    std::lock_guard guard(mutex);

    // Create descriptors
    segment->patchDeviceCPUDescriptor = sharedCPUHeapAllocator->Allocate(1);
    segment->patchDeviceGPUDescriptor = sharedGPUHeapAllocator->Allocate(1);

    // Counter to be initialized
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

    // Pop a new command list
    segment->immediatePrePatch = queueState->PopCommandList();
    
    // Ease of use
    ID3D12GraphicsCommandList* patchList = segment->immediatePrePatch.commandList;

    // Set heap
    patchList->SetDescriptorHeaps(1u, &sharedGPUHeap);

    // Has the counter data been initialized?
    //   Only required once per segment allocation, as the segments are recycled this usually
    //   only occurs during application startup.
    if (segment->allocation->pendingInitialization) {
        uint32_t clearValue[4] = { 0x0, 0x0, 0x0, 0x0 };

        // Clear device counters
        patchList->ClearUnorderedAccessViewUint(
            segment->patchDeviceGPUDescriptor.gpuHandle, segment->patchDeviceCPUDescriptor.cpuHandle,
            counter.allocation.device.resource,
            clearValue,
            0u,
            nullptr
        );
        
        // Flush all pending work and transition to src
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource = counter.allocation.device.resource;
        patchList->ResourceBarrier(1u, &barrier);

        // Mark as initialized
        segment->allocation->pendingInitialization = false;
    }

    // Patch all referenced heaps
    // Note: Internally dirty tested, duplicates are fine
    for (DescriptorHeapState* state : segment->referencedHeaps) {
        if (state->prmTable) {
            state->prmTable->Update(patchList);
        }
    }

    // Done
    HRESULT hr = patchList->Close();
    if (FAILED(hr)) {
        return nullptr;
    }

    // OK
    return patchList;
}

ID3D12GraphicsCommandList* ShaderExportStreamer::RecordPostCommandList(CommandQueueState* queueState, ShaderExportStreamSegment* segment) {
    std::lock_guard guard(mutex);
    
    // Counter to be copied
    const ShaderExportSegmentCounterInfo& counter = segment->allocation->counter;

    // Pop a new command list
    segment->immediatePostPatch = queueState->PopCommandList();
    
    // Ease of use
    ID3D12GraphicsCommandList* patchList = segment->immediatePostPatch.commandList;

    // Set heap
    patchList->SetDescriptorHeaps(1u, &sharedGPUHeap);

    // Flush all pending work and transition to src
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = counter.allocation.device.resource;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    patchList->ResourceBarrier(1u, &barrier);

    // Copy the counter from device to host
    patchList->CopyBufferRegion(
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
    patchList->ResourceBarrier(1u, &barrier);

    uint32_t clearValue[4] = { 0x0, 0x0, 0x0, 0x0 };

    // Clear device counters
    patchList->ClearUnorderedAccessViewUint(
        segment->patchDeviceGPUDescriptor.gpuHandle, segment->patchDeviceCPUDescriptor.cpuHandle,
        counter.allocation.device.resource,
        clearValue,
        0u,
        nullptr
    );

    // Done
    HRESULT hr = patchList->Close();
    if (FAILED(hr)) {
        return nullptr;
    }

    // OK
    return patchList;
}
