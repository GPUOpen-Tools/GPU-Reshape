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
#include <Backends/DX12/Programs/Programs.h>
#include <Backends/DX12/Raytracing.h>
#include <Backends/DX12/CommandList.h>
#include <Backends/DX12/Table.Gen.h>

// Shared
#include <Shared/ShaderRecordPatching.h>

// TODO[rt]: This signature is a bit messy
static void PatchShaderRecordsRegionDWords(
    DeviceTable& device, CommandListState* state,
    const StateObjectState* pipeline,
    StateObjectShaderIdentifierPatch* patchTable,
    const ShaderExportOwnedHeapAllocation& heapAllocation,
    ID3D12Resource* sharedAllocation, uint64_t descriptorOffset, uint64_t descriptorLength,
    const D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE& patched,
    const D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE& source) {
    // No records? Nothing to patch
    if (source.SizeInBytes == 0) {
        return;
    }

    // Find the resource for the source records
    ResourceState* sourceResource = device.state->virtualAddressTable.Find(source.StartAddress);

    // Total number of records
    uint64_t recordCount = source.SizeInBytes / source.StrideInBytes;
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

    /** For offsets see RaytracingBindingTablePatching.hlsl */

    // Create constant CBV
    D3D12_CONSTANT_BUFFER_VIEW_DESC constantView{};
    constantView.BufferLocation = constantAllocation.resource->GetGPUVirtualAddress() + constantAllocation.offset;
    constantView.SizeInBytes = constantSize;
    device.state->object->CreateConstantBufferView(&constantView, heapAllocation.CPU(0));

    // Create source dwords SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC sbtSourceDwordsDesc{};
    sbtSourceDwordsDesc.Format = DXGI_FORMAT_R32_UINT;
    sbtSourceDwordsDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sbtSourceDwordsDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sbtSourceDwordsDesc.Buffer.FirstElement = (source.StartAddress - sourceResource->object->GetGPUVirtualAddress()) / sizeof(uint32_t);
    sbtSourceDwordsDesc.Buffer.NumElements = static_cast<UINT>(source.SizeInBytes / sizeof(uint32_t));
    device.state->object->CreateShaderResourceView(sourceResource->object, &sbtSourceDwordsDesc, heapAllocation.CPU(1));

    // Create patched dwords UAV
    D3D12_UNORDERED_ACCESS_VIEW_DESC sbtPatchedDwordsDesc{};
    sbtPatchedDwordsDesc.Format = DXGI_FORMAT_R32_UINT;
    sbtPatchedDwordsDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    sbtPatchedDwordsDesc.Buffer.FirstElement = (patched.StartAddress - sharedAllocation->GetGPUVirtualAddress()) / sizeof(uint32_t);
    sbtPatchedDwordsDesc.Buffer.NumElements = static_cast<UINT>(patched.SizeInBytes / sizeof(uint32_t));
    device.state->object->CreateUnorderedAccessView(sharedAllocation, nullptr, &sbtPatchedDwordsDesc, heapAllocation.CPU(2));

    // Create descriptor data UAV
    D3D12_UNORDERED_ACCESS_VIEW_DESC descriptorDesc{};
    descriptorDesc.Format = DXGI_FORMAT_R32_UINT;
    descriptorDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    descriptorDesc.Buffer.FirstElement = descriptorOffset / sizeof(uint32_t);
    descriptorDesc.Buffer.NumElements = std::max<uint32_t>(static_cast<UINT>((source.StrideInBytes - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) * recordCount) / sizeof(uint32_t), 1u);
    device.state->object->CreateUnorderedAccessView(sharedAllocation, nullptr, &descriptorDesc, heapAllocation.CPU(3)); 

    // Create identifier table SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC sbtIdentifierTable{};
    sbtIdentifierTable.Format = DXGI_FORMAT_R32G32_UINT;
    sbtIdentifierTable.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sbtIdentifierTable.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sbtIdentifierTable.Buffer.NumElements = static_cast<UINT>(pipeline->identifierTable->tableCount);
    device.state->object->CreateShaderResourceView(pipeline->identifierTable->tableAllocation.resource, &sbtIdentifierTable, heapAllocation.CPU(4));

    // Create identifier list SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC sbtIdentifierList{};
    sbtIdentifierList.Format = DXGI_FORMAT_UNKNOWN;
    sbtIdentifierList.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sbtIdentifierList.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sbtIdentifierList.Buffer.NumElements = static_cast<UINT>(pipeline->identifierExports.size());
    sbtIdentifierList.Buffer.StructureByteStride = static_cast<UINT>(sizeof(SBTIdentifierTableEntry));
    device.state->object->CreateShaderResourceView(pipeline->identifierTable->listAllocation.resource, &sbtIdentifierList, heapAllocation.CPU(5));

    // Create identifier patch SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC sbtIdentifierPatchTable{};
    sbtIdentifierPatchTable.Format = DXGI_FORMAT_UNKNOWN;
    sbtIdentifierPatchTable.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sbtIdentifierPatchTable.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sbtIdentifierPatchTable.Buffer.NumElements = static_cast<UINT>(pipeline->identifierExports.size());
    sbtIdentifierPatchTable.Buffer.StructureByteStride = static_cast<UINT>(sizeof(SBTIdentifierPatch));
    device.state->object->CreateShaderResourceView(patchTable->listAllocation.resource, &sbtIdentifierPatchTable, heapAllocation.CPU(6));

    // Dispatch the patcher
    state->object->SetComputeRootDescriptorTable(0u, heapAllocation.gpu);
    state->object->Dispatch(static_cast<UINT>((recordCount + 31) / 32), 1, 1);
}

static void AlignInPlace(uint64_t& value, uint64_t align) {
    value = (value + align - 1) & ~(align - 1);
}

static void PatchShaderRecordsSourceImplicits(D3D12_DISPATCH_RAYS_DESC& desc) {
    // TODO[rt]: We're patching the strides... But this is actually valid if we dont want to advance the record, fix it!
    
    if (!desc.HitGroupTable.StrideInBytes) {
        desc.HitGroupTable.StrideInBytes = desc.HitGroupTable.SizeInBytes;
    }

    if (!desc.MissShaderTable.StrideInBytes) {
        desc.MissShaderTable.StrideInBytes = desc.MissShaderTable.SizeInBytes;
    }

    if (!desc.CallableShaderTable.StrideInBytes) {
        desc.CallableShaderTable.StrideInBytes = desc.CallableShaderTable.SizeInBytes;
    }
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

    // Patch the implicits
    PatchShaderRecordsSourceImplicits(desc);

    // We really just need two dwords, but alignment requirements mean that we have to increment by the full alignment
    uint32_t recordUdStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
    
    // Patched description
    D3D12_DISPATCH_RAYS_DESC patched{};
    patched.Width = desc.Width;
    patched.Height = desc.Height;
    patched.Depth = desc.Depth;

    // Set ray-gen indexing
    patched.RayGenerationShaderRecord.SizeInBytes = desc.RayGenerationShaderRecord.SizeInBytes + recordUdStride;

    // Set callable indexing
    patched.CallableShaderTable.StrideInBytes = desc.CallableShaderTable.StrideInBytes + recordUdStride;
    patched.CallableShaderTable.SizeInBytes = (desc.CallableShaderTable.SizeInBytes / std::max(desc.CallableShaderTable.StrideInBytes, 1ull)) * patched.CallableShaderTable.StrideInBytes;

    // Set hit indexing
    patched.HitGroupTable.StrideInBytes = desc.HitGroupTable.StrideInBytes + recordUdStride;
    patched.HitGroupTable.SizeInBytes = (desc.HitGroupTable.SizeInBytes / std::max(desc.HitGroupTable.StrideInBytes, 1ull)) * patched.HitGroupTable.StrideInBytes;

    // Set miss properties
    patched.MissShaderTable.StrideInBytes = desc.MissShaderTable.StrideInBytes + recordUdStride;
    patched.MissShaderTable.SizeInBytes = (desc.MissShaderTable.SizeInBytes / std::max(desc.MissShaderTable.StrideInBytes, 1ull)) * patched.MissShaderTable.StrideInBytes;

    // Descriptor lengths
    uint64_t rayGenDescriptorLength   = (desc.RayGenerationShaderRecord.SizeInBytes - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    uint64_t callableDescriptorLength = (desc.CallableShaderTable.StrideInBytes     - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) * (desc.CallableShaderTable.SizeInBytes / std::max(desc.CallableShaderTable.StrideInBytes, 1ull));
    uint64_t hitDescriptorLength      = (desc.HitGroupTable.StrideInBytes           - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) * (desc.HitGroupTable.SizeInBytes / std::max(desc.HitGroupTable.StrideInBytes, 1ull));
    uint64_t missDescriptorLength     = (desc.MissShaderTable.StrideInBytes         - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) * (desc.MissShaderTable.SizeInBytes / std::max(desc.MissShaderTable.StrideInBytes, 1ull));

    // Total allocation size for patched records
    uint64_t patchedAllocationSize = 0;
    AlignInPlace(patchedAllocationSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    patchedAllocationSize += patched.RayGenerationShaderRecord.SizeInBytes;
    AlignInPlace(patchedAllocationSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    patchedAllocationSize += patched.CallableShaderTable.SizeInBytes;
    AlignInPlace(patchedAllocationSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    patchedAllocationSize += patched.HitGroupTable.SizeInBytes;
    AlignInPlace(patchedAllocationSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    patchedAllocationSize += patched.MissShaderTable.SizeInBytes;
    patchedAllocationSize += rayGenDescriptorLength;
    patchedAllocationSize += callableDescriptorLength;
    patchedAllocationSize += hitDescriptorLength;
    patchedAllocationSize += missDescriptorLength;

    // Terminator for zero sized buffers
    patchedAllocationSize += sizeof(uint32_t);

    // Allocate the shared allocation, we suboffset into this
    ShaderExportDeviceAllocation allocation = state->streamState->deviceAllocator.Allocate(device.state->deviceAllocator, patchedAllocationSize);

    // Current patched offset
    uint64_t patchedOffset = 0;

    // Offset into ray generation
    AlignInPlace(patchedOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    patched.RayGenerationShaderRecord.StartAddress = allocation.allocation.resource->GetGPUVirtualAddress() + patchedOffset;
    patchedOffset += patched.RayGenerationShaderRecord.SizeInBytes;

    // Offset into callable
    AlignInPlace(patchedOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    patched.CallableShaderTable.StartAddress = allocation.allocation.resource->GetGPUVirtualAddress() + patchedOffset;
    patchedOffset += patched.CallableShaderTable.SizeInBytes;

    // Offset into hits
    AlignInPlace(patchedOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    patched.HitGroupTable.StartAddress = allocation.allocation.resource->GetGPUVirtualAddress() + patchedOffset;
    patchedOffset += patched.HitGroupTable.SizeInBytes;

    // Offset into misses
    AlignInPlace(patchedOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    patched.MissShaderTable.StartAddress = allocation.allocation.resource->GetGPUVirtualAddress() + patchedOffset;
    patchedOffset += patched.MissShaderTable.SizeInBytes;

    // Offset into raygen descriptor data
    uint64_t rayGenDescriptorOffset = patchedOffset;
    patchedOffset += rayGenDescriptorLength;
    
    // Offset into callable descriptor data
    uint64_t callableDescriptorOffset = patchedOffset;
    patchedOffset += callableDescriptorLength;

    // Offset into hit descriptor data
    uint64_t hitDescriptorOffset = patchedOffset;
    patchedOffset += hitDescriptorLength;

    // Offset into miss descriptor data
    uint64_t missDescriptorOffset = patchedOffset;
    patchedOffset += missDescriptorLength;

    // Validate expected offsets
    ASSERT(patchedOffset == patchedAllocationSize - sizeof(uint32_t), "Unexpected offset");

    // We need to patch 4 separate regions each with 7 descriptors
    // TODO[rt]: Magical constants!
    uint32_t dispatchCount   = 4;
    uint32_t descriptorCount = 7;

    // Create a single descriptor heap allocation, allows us to share the heap instead of constantly bouncing
    ShaderExportOwnedHeapAllocation heapAllocation = state->streamState->heapAllocator.Allocate(device.state, dispatchCount * descriptorCount);

    // Setup command state
    state->object->SetDescriptorHeaps(1u, &heapAllocation.heap);
    state->object->SetComputeRootSignature(device.state->programs->raytracingBindingTablePatch.rootSignature);
    state->object->SetPipelineState(device.state->programs->raytracingBindingTablePatch.pipelineState);

    // Patch all ray generation records
    PatchShaderRecordsRegionDWords(
        device, state, pipeline, patchTable,
        heapAllocation.Advance(descriptorCount * 0),
        allocation.allocation.resource, rayGenDescriptorOffset, rayGenDescriptorLength,
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
        device, state, pipeline, patchTable,
        heapAllocation.Advance(descriptorCount * 1),
        allocation.allocation.resource, callableDescriptorOffset, callableDescriptorLength,
        patched.CallableShaderTable, desc.CallableShaderTable
    );

    // Patch all hit records
    PatchShaderRecordsRegionDWords(
        device, state, pipeline, patchTable,
        heapAllocation.Advance(descriptorCount * 2),
        allocation.allocation.resource, hitDescriptorOffset, hitDescriptorLength,
        patched.HitGroupTable, desc.HitGroupTable
    );

    // Patch all miss records
    PatchShaderRecordsRegionDWords(
        device, state, pipeline, patchTable,
        heapAllocation.Advance(descriptorCount * 3),
        allocation.allocation.resource, missDescriptorOffset, missDescriptorLength,
        patched.MissShaderTable, desc.MissShaderTable
    );

    // Reconstruct the previous command state
    ReconstructState(device.state, state->object, state->streamState);

    // Barrier for next commands
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = nullptr;
    state->object->ResourceBarrier(1u, &barrier);
    
    // OK
    return patched;
}

static D3D12_DISPATCH_RAYS_DESC PatchShaderRecords(DeviceTable& device, CommandListState* state, const D3D12_DISPATCH_RAYS_DESC& Desc) {
    // TODO[rt]: Actual caching
    return PatchShaderRecordsImmediate(device, state, Desc);
}

void HookID3D12CommandListDispatchRays(ID3D12GraphicsCommandList4* list, const D3D12_DISPATCH_RAYS_DESC* pDesc) {
    auto table = GetTable(list);

    // Get device
    auto device = GetTable(table.state->parent);

    // Patch all relevant records
    D3D12_DISPATCH_RAYS_DESC patched;
    if (table.state->streamState->isInstrumented) {
        patched = PatchShaderRecords(device, table.state, *pDesc);
    } else {
        patched = *pDesc;
    }

    // Commit all pending compute
    CommitCompute(device.state, table.state);

    // Pass down callchain
    table.next->DispatchRays(&patched);
}

