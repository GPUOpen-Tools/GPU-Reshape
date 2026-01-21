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

// Layer
#include <Backends/DX12/Export/ShaderExportFixedTwoSidedDescriptorAllocator.h>
#include <Backends/DX12/Export/ShaderExportStreamState.h>
#include <Backends/DX12/Export/ShaderExportStreamStateBarrierTracking.h>
#include <Backends/DX12/Export/ShaderExportStreamStateRaytracingCache.h>
#include <Backends/DX12/Programs/Programs.h>
#include <Backends/DX12/Raytracing.h>
#include <Backends/DX12/CommandList.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/Controllers/ConfigController.h>
#include <Backends/DX12/Export/ShaderExportStreamer.h>

// Shared
#include <Shared/ShaderRecordPatching.h>
#include <Shared/RaytracingIndirectSetup.h>

// TODO[rt]: This signature is a bit messy
static void PatchShaderRecordsRegionDWords(
    DeviceTable& device, CommandListState* state,
    const StateObjectState* pipeline,
    D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable,
    ID3D12Resource* sharedAllocation, uint64_t descriptorOffset, uint64_t descriptorLength,
    const D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE& patched,
    const D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE& source) {
    // No records? Nothing to patch
    if (source.SizeInBytes == 0) {
        return;
    }

    // Total number of records
    uint64_t sourceStrideOrSize = source.StrideInBytes ? source.StrideInBytes : source.SizeInBytes;
    uint64_t patchedStrideOrSize = patched.StrideInBytes ? patched.StrideInBytes : patched.SizeInBytes;
    uint64_t recordCount = source.SizeInBytes / sourceStrideOrSize;
    uint64_t descriptorStride = descriptorLength / recordCount;

    // Patch constants size
    uint32_t constantAlignSub1 = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1;
    uint32_t constantSize = (static_cast<uint32_t>(sizeof(SBTPatchConstantData)) + constantAlignSub1) & ~constantAlignSub1;

    // Allocate the patch constants separately, avoids staging needs
    ShaderExportConstantAllocation constantAllocation = state->streamState->constantAllocator.Allocate(device.state->deviceAllocator, constantSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    // Fill constants
    auto* data = static_cast<SBTPatchConstantData*>(constantAllocation.staging);
    data->ResourceHeapOffset = state->streamState->resourceHeap ? state->streamState->resourceHeap->object->GetGPUDescriptorHandleForHeapStart().ptr : 0ull;
    data->SamplerHeapOffset = state->streamState->samplerHeap ? state->streamState->samplerHeap->object->GetGPUDescriptorHandleForHeapStart().ptr : 0ull;
    data->ResourceHeapStride = static_cast<uint>(state->streamState->resourceHeap->stride);
    data->SamplerHeapStride = static_cast<uint>(state->streamState->resourceHeap->stride);
    data->SourceDWordStride = static_cast<uint>(source.StrideInBytes / sizeof(uint32_t));
    data->PatchedDWordStride = static_cast<uint>(patched.StrideInBytes / sizeof(uint32_t));
    data->DescriptorConstantStart = sharedAllocation->GetGPUVirtualAddress() + descriptorOffset;
    data->DescriptorConstantStride = static_cast<uint>(descriptorStride / sizeof(uint32_t));
    data->SBTIdentifierTableSize = static_cast<uint>(pipeline->identifierTable->tableCount);
    data->SBTRecordCount = static_cast<uint>(recordCount);
    
    // Get backend messages
    ID3D12Resource* backendMessages = GetStreamingStateBackendMessages(state);
    
    // Set immutable
    state->object->SetComputeRootDescriptorTable(0u, descriptorTable);

    // Set mutable
    state->object->SetComputeRootConstantBufferView(1u, constantAllocation.resource->GetGPUVirtualAddress() + constantAllocation.offset);
    state->object->SetComputeRootShaderResourceView(2u, source.StartAddress);
    state->object->SetComputeRootUnorderedAccessView(3u, patched.StartAddress);
    state->object->SetComputeRootUnorderedAccessView(4u, sharedAllocation->GetGPUVirtualAddress() + descriptorOffset);
    state->object->SetComputeRootUnorderedAccessView(5u, backendMessages->GetGPUVirtualAddress());

    // Dispatch the patcher
    state->object->Dispatch(static_cast<UINT>((recordCount + 31) / 32), 1, 1);
}

static void CreateImmutablePatchDescriptors(DeviceTable& device, const StateObjectState* pipeline, StateObjectShaderIdentifierPatch* patchTable, const ShaderExportOwnedHeapAllocation& heapAllocation) {
    /** For offsets see RaytracingBindingTablePatching.hlsl */
    
    // Create identifier table SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC sbtIdentifierTable{};
    sbtIdentifierTable.Format = DXGI_FORMAT_R32G32_UINT;
    sbtIdentifierTable.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sbtIdentifierTable.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sbtIdentifierTable.Buffer.NumElements = static_cast<UINT>(pipeline->identifierTable->tableCount);
    device.state->object->CreateShaderResourceView(pipeline->identifierTable->tableAllocation.resource, &sbtIdentifierTable, heapAllocation.CPU(0));

    // Create identifier list SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC sbtIdentifierList{};
    sbtIdentifierList.Format = DXGI_FORMAT_UNKNOWN;
    sbtIdentifierList.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sbtIdentifierList.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sbtIdentifierList.Buffer.NumElements = static_cast<UINT>(pipeline->identifierExports.size());
    sbtIdentifierList.Buffer.StructureByteStride = static_cast<UINT>(sizeof(SBTIdentifierTableEntry));
    device.state->object->CreateShaderResourceView(pipeline->identifierTable->listAllocation.resource, &sbtIdentifierList, heapAllocation.CPU(1));

    // Create identifier patch SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC sbtIdentifierPatchTable{};
    sbtIdentifierPatchTable.Format = DXGI_FORMAT_UNKNOWN;
    sbtIdentifierPatchTable.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sbtIdentifierPatchTable.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sbtIdentifierPatchTable.Buffer.NumElements = static_cast<UINT>(pipeline->identifierExports.size());
    sbtIdentifierPatchTable.Buffer.StructureByteStride = static_cast<UINT>(sizeof(SBTIdentifierPatch));
    device.state->object->CreateShaderResourceView(patchTable->listAllocation.resource, &sbtIdentifierPatchTable, heapAllocation.CPU(2));
}

static D3D12_DISPATCH_RAYS_DESC PatchShaderRecordsImmediate(DeviceTable& device, CommandListState* state, D3D12_DISPATCH_RAYS_DESC desc) {
    // Get the state object
    ASSERT(state->streamState->pipeline->type == PipelineType::StateObject, "Unexpected pipeline state");
    auto pipeline = static_cast<const StateObjectState*>(state->streamState->pipeline);

    // Get the current hot patch table
    // TODO[rt]: Dont keep a current patch table, just index by the current hash index instead somehow
    StateObjectShaderIdentifierPatch *patchTable = pipeline->hotSwapPatchTable.load();
    if (!patchTable) {
        return desc;
    }

    // Create context
    SBTSharedAllocationContext ctx = SBTContextCreate();

    // Determine the number of bytes needed
    uint64_t patchedAllocationSize = SBTContextSetup(ctx, desc);

    // Allocate the shared allocation, we suboffset into this
    ShaderExportDeviceAllocation allocation = state->streamState->deviceAllocator.Allocate(device.state->deviceAllocator, patchedAllocationSize);

    // Patch the dispatch
    D3D12_DISPATCH_RAYS_DESC patched = SBTContextPatch(ctx, allocation.allocation.resource->GetGPUVirtualAddress());

    // Create a single descriptor heap allocation, allows us to share the heap instead of constantly bouncing
    ShaderExportOwnedHeapAllocation heapAllocation = state->streamState->heapAllocator.Allocate(device.state, 4u);
    CreateImmutablePatchDescriptors(device, pipeline, patchTable, heapAllocation);
    
    // Mark the last descriptor as VAMT
    // TODO: These magic offsets are a terrible idea
    state->streamState->persistentState.vamtVersionDescriptorHandles.Add(heapAllocation.CPU(3));

    // Setup command state
    state->object->SetDescriptorHeaps(1u, &heapAllocation.heap);
    state->object->SetComputeRootSignature(device.state->programs->raytracingBindingTablePatch.sbtPatchRootSignature);
    state->object->SetPipelineState(device.state->programs->raytracingBindingTablePatch.sbtPatchPipelineState);

    // Patch all ray generation records
    PatchShaderRecordsRegionDWords(
        device, state, pipeline,
        heapAllocation.GPU(),
        allocation.allocation.resource, ctx.RayGenDescriptorOffset, ctx.RayGenDescriptorLength,
        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{
            .StartAddress = patched.RayGenerationShaderRecord.StartAddress,
            .SizeInBytes = patched.RayGenerationShaderRecord.SizeInBytes,
            .StrideInBytes = patched.RayGenerationShaderRecord.SizeInBytes
        },
        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{
            .StartAddress = desc.RayGenerationShaderRecord.StartAddress,
            .SizeInBytes = desc.RayGenerationShaderRecord.SizeInBytes,
            .StrideInBytes = desc.RayGenerationShaderRecord.SizeInBytes
        }
    );

    // Patch all callable records
    PatchShaderRecordsRegionDWords(
        device, state, pipeline, 
        heapAllocation.GPU(),
        allocation.allocation.resource, ctx.CallableDescriptorOffset, ctx.CallableDescriptorLength,
        patched.CallableShaderTable, desc.CallableShaderTable
    );

    // Patch all hit records
    PatchShaderRecordsRegionDWords(
        device, state, pipeline,
        heapAllocation.GPU(),
        allocation.allocation.resource, ctx.HitDescriptorOffset, ctx.HitDescriptorLength,
        patched.HitGroupTable, desc.HitGroupTable
    );

    // Patch all miss records
    PatchShaderRecordsRegionDWords(
        device, state, pipeline, 
        heapAllocation.GPU(),
        allocation.allocation.resource, ctx.MissDescriptorOffset, ctx.MissDescriptorLength,
        patched.MissShaderTable, desc.MissShaderTable
    );

    // Barrier for next commands
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = nullptr;
    state->object->ResourceBarrier(1u, &barrier);

    // OK
    return patched;
}

static ID3D12Resource* CopyIndirectArgumentCommandBuffer(DeviceTable& device, CommandListState* state, CommandSignatureTable& signatureTable, UINT MaxCommandCount, ID3D12Resource *pArgumentBuffer, UINT64 ArgumentBufferOffset) {
    pArgumentBuffer = ConditionalNext(pArgumentBuffer);
    
    // We're going to copy over the command data
    ShaderExportDeviceAllocation commandAllocation = state->streamState->deviceAllocator.Allocate(device.state->deviceAllocator, signatureTable.state->byteStride * MaxCommandCount);

    // Argument Buffer -> Copy Source
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = pArgumentBuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    state->object->ResourceBarrier(1u, &barrier);

    // Copy over all data
    state->object->CopyBufferRegion(
        commandAllocation.allocation.resource, 0,
        pArgumentBuffer, ArgumentBufferOffset,
        commandAllocation.length
    );
    
    // Argument Buffer -> Indirect
    barrier.Transition.pResource = pArgumentBuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    state->object->ResourceBarrier(1u, &barrier);
    
    // Copy Buffer -> Indirect
    barrier.Transition.pResource = commandAllocation.allocation.resource;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    state->object->ResourceBarrier(1u, &barrier);

    // OK
    return commandAllocation.allocation.resource;
}

static ID3D12Resource* PatchShaderRecordsImmediateIndirect(
    DeviceTable& device, CommandListState* state, ID3D12CommandSignature *pCommandSignature,
    UINT MaxCommandCount, ID3D12Resource *pArgumentBuffer, UINT64 ArgumentBufferOffset,
    ID3D12Resource *pCountBuffer, UINT64 CountBufferOffset) {
    auto signatureTable = GetTable(pCommandSignature);

    // Get the state object
    ASSERT(state->streamState->pipeline->type == PipelineType::StateObject, "Unexpected pipeline state");
    auto pipeline = static_cast<const StateObjectState*>(state->streamState->pipeline);

    // Get the current hot patch table
    // TODO[rt]: Dont keep a current patch table, just index by the current hash index instead somehow
    StateObjectShaderIdentifierPatch *patchTable = pipeline->hotSwapPatchTable.load();
    if (!patchTable) {
        return nullptr;
    }

    // Assign a dummy count if needed
    if (!pCountBuffer) {
        // Just write MAX
        ShaderExportConstantAllocation dummyCount = state->streamState->constantAllocator.Allocate(device.state->deviceAllocator, sizeof(uint32_t), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        *static_cast<uint32_t*>(dummyCount.staging) = UINT32_MAX;

        // Replace count
        pCountBuffer      = dummyCount.resource;
        CountBufferOffset = dummyCount.offset;
    }

    // Get backend messages
    ID3D12Resource* backendMessages = GetStreamingStateBackendMessages(state);
    
    // Copy over all the commands
    ID3D12Resource* commandBuffer = CopyIndirectArgumentCommandBuffer(device, state, signatureTable, MaxCommandCount, pArgumentBuffer, ArgumentBufferOffset);

    // Create immutable descriptors
    ShaderExportOwnedHeapAllocation heapAllocation = state->streamState->heapAllocator.Allocate(device.state, 3u);
    CreateImmutablePatchDescriptors(device, pipeline, patchTable, heapAllocation);

    // Set common heap
    state->object->SetDescriptorHeaps(1u, &heapAllocation.heap);
    
    // Handle all commands
    // Note: This is the theoretical limit, not the actual number of commands
    // TODO[rt]: We **could** do this in a single EI, though it would complicate some things
    for (uint32_t commandIndex = 0; commandIndex < MaxCommandCount; commandIndex++) {
        static_assert(sizeof(SBTPatchConstantData) == D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, "Unexpected size");

        // Always the last command
        uint64_t commandOffset    = signatureTable.state->byteStride * commandIndex * signatureTable.state->arguments.size();
        uint64_t subCommandOffset = commandOffset + signatureTable.state->byteStride * (signatureTable.state->arguments.size() - 1);
        ASSERT(signatureTable.state->arguments.back().Type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS, "Unexpected command type");

        // We're now in an indirect ray dispatch, allocate scratch space
        ShaderExportDeviceAllocation scratchAllocation = state->streamState->deviceAllocator.Allocate(
            device.state->deviceAllocator,
            device.state->configController->indirect.scratchByteCount
        );

        // Allocate constant data, filled out in device memory
        ShaderExportDeviceAllocation patchConstantAllocation = state->streamState->deviceAllocator.Allocate(
            device.state->deviceAllocator,
            D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * RaytracingIndirectSetupCommandRangeCount
        );

        // Allocate EI signature data
        ShaderExportDeviceAllocation signatureAllocation = state->streamState->deviceAllocator.Allocate(
            device.state->deviceAllocator,
            RaytracingIndirectSetupCommandByteStride * RaytracingIndirectSetupCommandRangeCount
        );

        // Allocate the patch constants separately, avoids staging needs
        ShaderExportConstantAllocation constantAllocation = state->streamState->constantAllocator.Allocate(device.state->deviceAllocator, sizeof(SBTIndirectSetupConstantData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        
        // Fill constants
        auto* data = static_cast<SBTIndirectSetupConstantData*>(constantAllocation.staging);
        data->ScratchByteCount = device.state->configController->indirect.scratchByteCount;
        data->ConstantStride = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        data->CommandIndex = commandIndex;
        data->ScratchBaseAddress = scratchAllocation.allocation.resource->GetGPUVirtualAddress();
        data->ConstantBaseAddress = patchConstantAllocation.allocation.resource->GetGPUVirtualAddress();

        // Fill common patch constants
        data->CommonPatchConstants.ResourceHeapOffset = state->streamState->resourceHeap ? state->streamState->resourceHeap->object->GetGPUDescriptorHandleForHeapStart().ptr : 0ull;
        data->CommonPatchConstants.SamplerHeapOffset = state->streamState->samplerHeap ? state->streamState->samplerHeap->object->GetGPUDescriptorHandleForHeapStart().ptr : 0ull;
        data->CommonPatchConstants.ResourceHeapStride = static_cast<uint>(state->streamState->resourceHeap->stride);
        data->CommonPatchConstants.SamplerHeapStride = static_cast<uint>(state->streamState->resourceHeap->stride);
        data->CommonPatchConstants.SBTIdentifierTableSize = static_cast<uint>(pipeline->identifierTable->tableCount);

        // Setup command state
        state->object->SetComputeRootSignature(device.state->programs->raytracingBindingTablePatch.raytracingIndirectSetupRootSignature);
        state->object->SetPipelineState(device.state->programs->raytracingBindingTablePatch.raytracingIndirectSetupPipelineState);

        // Bind parameters
        state->object->SetComputeRootConstantBufferView(0, constantAllocation.resource->GetGPUVirtualAddress() + constantAllocation.offset);
        state->object->SetComputeRootUnorderedAccessView(1, commandBuffer->GetGPUVirtualAddress() + subCommandOffset);
        state->object->SetComputeRootUnorderedAccessView(2, backendMessages->GetGPUVirtualAddress());
        state->object->SetComputeRootUnorderedAccessView(3, signatureAllocation.allocation.resource->GetGPUVirtualAddress());
        state->object->SetComputeRootUnorderedAccessView(4, patchConstantAllocation.allocation.resource->GetGPUVirtualAddress());
        state->object->SetComputeRootShaderResourceView(5, pCountBuffer->GetGPUVirtualAddress() + CountBufferOffset);

        // Write the indirect arguments and patch the command buffer data
        state->object->Dispatch(1, 1, 1);

        // Argument Buffer -> Indirect
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = signatureAllocation.allocation.resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        state->object->ResourceBarrier(1u, &barrier);

        // Setup command state
        state->object->SetComputeRootSignature(device.state->programs->raytracingBindingTablePatch.sbtPatchRootSignature);
        state->object->SetPipelineState(device.state->programs->raytracingBindingTablePatch.sbtPatchPipelineState);

        // Set immutable, the mutable parameters are handled by the EI
        state->object->SetComputeRootDescriptorTable(0u, heapAllocation.GPU());
        
        // Finally, perform the SBT patching
        // TODO[rt]: Batch all of these, avoids the constant bouncing
        state->object->ExecuteIndirect(
            device.state->programs->raytracingBindingTablePatch.raytracingIndirectSetupCommandSignature, RaytracingIndirectSetupCommandRangeCount,
            signatureAllocation.allocation.resource, 0,
            nullptr, 0
        );
    }
    
    // Command Buffer -> Indirect
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = commandBuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    state->object->ResourceBarrier(1u, &barrier);
        
    // Finally, let the hook invoke the arguments as if it was the same
    return commandBuffer;
}

static void CombineHash(uint64_t& hash, const D3D12_GPU_VIRTUAL_ADDRESS_RANGE& desc) {
    CombineHash(hash, desc.StartAddress);
    CombineHash(hash, desc.SizeInBytes);
}

static void CombineHash(uint64_t& hash, const D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE& desc) {
    CombineHash(hash, desc.StartAddress);
    CombineHash(hash, desc.SizeInBytes);
    CombineHash(hash, desc.StrideInBytes);
}

static void CombineHash(uint64_t& hash, const D3D12_DISPATCH_RAYS_DESC& desc) {
    CombineHash(hash, desc.RayGenerationShaderRecord);
    CombineHash(hash, desc.MissShaderTable);
    CombineHash(hash, desc.HitGroupTable);
    CombineHash(hash, desc.CallableShaderTable);
    CombineHash(hash, desc.Width);
    CombineHash(hash, desc.Height);
    CombineHash(hash, desc.Depth);
}

void AddCachedNativeObject(CommandListState* state, ShaderExportStreamStateRaytracingPatchEntry& entry, ResourceState* resource) {
    // Deduplicate, faster lookup times
    for (ResourceState* other : entry.resources) {
        if (other == resource) {
            return;
        }
    }

    // Not found, add
    entry.resources.Add(resource);

    // Mark as tracked in raytracing
    ShaderExportStreamStateBarrierTracking *tracking = GetBarrierTracking(state);
    tracking->resources[resource] |= ShaderExportStreamStateBarrierFlag::Raytracing;
}

static D3D12_DISPATCH_RAYS_DESC PatchShaderRecords(DeviceTable& device, CommandListState* state, const D3D12_DISPATCH_RAYS_DESC& desc) {
    ShaderExportStreamStateRaytracingCache *cache = GetRaytracingCache(state);

    // Get hash
    uint64_t hash = 0;
    CombineHash(hash, desc);

    // Try to find cached entry
    for (const ShaderExportStreamStateRaytracingPatchEntry& entry : cache->patchEntries) {
        if (entry.hash == hash) {
            return entry.patched;
        }
    }

    // Patch the actual records
    D3D12_DISPATCH_RAYS_DESC patched = PatchShaderRecordsImmediate(device, state, desc);

    // Create cache entry
    ShaderExportStreamStateRaytracingPatchEntry& entry = cache->patchEntries.emplace_back();
    entry.hash = hash;
    entry.patched = patched;

    // Add all referenced native objects for barrier tracking
    AddCachedNativeObject(state, entry, device.state->virtualAddressTable.Find(desc.RayGenerationShaderRecord.StartAddress));
    AddCachedNativeObject(state, entry, device.state->virtualAddressTable.Find(desc.HitGroupTable.StartAddress));
    AddCachedNativeObject(state, entry, device.state->virtualAddressTable.Find(desc.MissShaderTable.StartAddress));
    AddCachedNativeObject(state, entry, device.state->virtualAddressTable.Find(desc.CallableShaderTable.StartAddress));

    // OK
    return patched;
}

void HookID3D12CommandListDispatchRays(ID3D12GraphicsCommandList4* list, const D3D12_DISPATCH_RAYS_DESC* pDesc) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Patch all relevant records
    D3D12_DISPATCH_RAYS_DESC patched;
    if (table.state->streamState->pipelineInstrument) {
        patched = PatchShaderRecords(device, table.state, *pDesc);

        // Reconstruct the previous command state
        ReconstructState(device.state, table.state->object, table.state->streamState);
    } else {
        patched = *pDesc;
    }

    // Commit all pending compute
    CommitCompute(device.state, table.state);

    // Pass down callchain
    table.next->DispatchRays(&patched);
}

void HookID3D12CommandListExecuteIndirectRaytracing(ID3D12CommandList *list, ID3D12CommandSignature *pCommandSignature, UINT MaxCommandCount, ID3D12Resource *pArgumentBuffer, UINT64 ArgumentBufferOffset, ID3D12Resource *pCountBuffer, UINT64 CountBufferOffset) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Get signature
    auto signatureTable = GetTable(pCommandSignature);

    // Patched data
    ID3D12Resource* patched = nullptr;

    // Offset into patched data
    // If patched, always zero as we copy the command buffer
    uint64_t patchedOffset = 0;

    // Instrumented?
    if (table.state->streamState->pipelineInstrument) {
        // We can't determine the actual resources used in the EI, so we can't do any kind of barrier tracking
        // to determine invalidation. So, always patch.
        patched = PatchShaderRecordsImmediateIndirect(
            device, table.state, pCommandSignature,
            MaxCommandCount,
            pArgumentBuffer, ArgumentBufferOffset,
            pCountBuffer, CountBufferOffset
        );

        // Reconstruct the previous command state
        ReconstructState(device.state, table.state->object, table.state->streamState);
    }

    // If not instrumented, or failed, pass through
    if (!patched) {
        patched       = ConditionalNext(pArgumentBuffer);
        patchedOffset = ArgumentBufferOffset;
    }

    // Commit compute
     device.state->exportStreamer->CommitCompute(table.state->streamState, table.state->object);

    // Finally, execute it
    table.next->ExecuteIndirect(
        signatureTable.next,
        MaxCommandCount,
        patched,
        patchedOffset,
        Next(pCountBuffer),
        CountBufferOffset
    );
}
