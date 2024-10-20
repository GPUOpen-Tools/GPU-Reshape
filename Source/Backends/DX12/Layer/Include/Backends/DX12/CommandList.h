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

#pragma once

// Backend
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/Layer.h>

// Forward declarations
struct DeviceState;

/// Create all device command proxies
/// \param state
void CreateDeviceCommandProxies(DeviceState *state);

/// Set the feature set
/// \param state
/// \param featureSet
void SetDeviceCommandFeatureSetAndCommit(DeviceState *state, uint64_t featureSet);

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateCommandQueue(ID3D12Device *, const D3D12_COMMAND_QUEUE_DESC *, const IID &, void **);
HRESULT WINAPI HookID3D12DeviceCreateCommandQueue1(ID3D12Device *, const D3D12_COMMAND_QUEUE_DESC* desc, const IID& creatorId, const IID& riid, void** ppCommandQueue);
HRESULT WINAPI HookID3D12DeviceCreateCommandSignature(ID3D12Device *, const D3D12_COMMAND_SIGNATURE_DESC* pDesc, ID3D12RootSignature* pRootSignature, const IID& riid, void** ppvCommandSignature);
HRESULT WINAPI HookID3D12CommandSignatureGetDevice(ID3D12CommandSignature*, const IID &, void **);
HRESULT WINAPI HookID3D12CommandQueueGetDevice(ID3D12CommandQueue *_this, REFIID riid, void **ppDevice);
HRESULT WINAPI HookID3D12DeviceCreateCommandAllocator(ID3D12Device *, D3D12_COMMAND_LIST_TYPE, const IID &, void **);
HRESULT WINAPI HookID3D12CommandAllocatorReset(ID3D12CommandAllocator *_this);
HRESULT WINAPI HookID3D12CommandAllocatorGetDevice(ID3D12CommandAllocator *_this, REFIID riid, void **ppDevice);
HRESULT WINAPI HookID3D12DeviceCreateCommandList(ID3D12Device *, UINT, D3D12_COMMAND_LIST_TYPE, ID3D12CommandAllocator *, ID3D12PipelineState *, const IID &, void **);
HRESULT WINAPI HookID3D12DeviceCreateCommandList1(ID3D12Device *, UINT, D3D12_COMMAND_LIST_TYPE, D3D12_COMMAND_LIST_FLAGS, const IID &, void **);
void WINAPI HookID3D12CommandQueueExecuteCommandLists(ID3D12CommandQueue *, UINT, ID3D12CommandList *const *);
void WINAPI HookID3D12CommandQueueGetDesc(ID3D12CommandQueue *, D3D12_COMMAND_QUEUE_DESC* out);
HRESULT WINAPI HookID3D12CommandQueueSignal(ID3D12CommandQueue *_this, ID3D12Fence *pFence, UINT64 Value);
HRESULT WINAPI HookID3D12CommandQueueWait(ID3D12CommandQueue *_this, ID3D12Fence *pFence, UINT64 Value);
void WINAPI HookID3D12CommandQueueCopyTileMappings(ID3D12CommandQueue* _this, ID3D12Resource* pDstResource, const D3D12_TILED_RESOURCE_COORDINATE* pDstRegionStartCoordinate, ID3D12Resource* pSrcResource, const D3D12_TILED_RESOURCE_COORDINATE* pSrcRegionStartCoordinate, const D3D12_TILE_REGION_SIZE* pRegionSize, D3D12_TILE_MAPPING_FLAGS Flags);
HRESULT WINAPI HookID3D12CommandListReset(ID3D12CommandList *list, ID3D12CommandAllocator *allocator, ID3D12PipelineState *state);
HRESULT WINAPI HookID3D12CommandListClose(ID3D12CommandList *list);
void WINAPI HookID3D12CommandListSetPipelineState(ID3D12CommandList *list, ID3D12PipelineState *pipeline);
void WINAPI HookID3D12CommandListSetGraphicsRootSignature(ID3D12CommandList *list, ID3D12RootSignature *rootSignature);
void WINAPI HookID3D12CommandListSetComputeRootSignature(ID3D12CommandList *list, ID3D12RootSignature *rootSignature);
void WINAPI HookID3D12CommandListSetDescriptorHeaps(ID3D12CommandList *list, UINT NumDescriptorHeaps, ID3D12DescriptorHeap *const *ppDescriptorHeaps);
void WINAPI HookID3D12CommandListSetComputeRootDescriptorTable(ID3D12CommandList *list, UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
void WINAPI HookID3D12CommandListSetGraphicsRootDescriptorTable(ID3D12CommandList *list, UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
void WINAPI HookID3D12CommandListSetComputeRootShaderResourceView(ID3D12CommandList *list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
void WINAPI HookID3D12CommandListSetGraphicsRootShaderResourceView(ID3D12CommandList *list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
void WINAPI HookID3D12CommandListSetComputeRootUnorderedAccessView(ID3D12CommandList *list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
void WINAPI HookID3D12CommandListSetGraphicsRootUnorderedAccessView(ID3D12CommandList *list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
void WINAPI HookID3D12CommandListSetComputeRootConstantBufferView(ID3D12CommandList *list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
void WINAPI HookID3D12CommandListSetGraphicsRootConstantBufferView(ID3D12CommandList *list, UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
void WINAPI HookID3D12CommandListCopyBufferRegion(ID3D12CommandList *list, ID3D12Resource* pDstBuffer, UINT64 DstOffset, ID3D12Resource* pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes);
void WINAPI HookID3D12CommandListCopyTextureRegion(ID3D12CommandList *list, const D3D12_TEXTURE_COPY_LOCATION *pDst, UINT DstX, UINT DstY, UINT DstZ, const D3D12_TEXTURE_COPY_LOCATION *pSrc, const D3D12_BOX *pSrcBox);
void WINAPI HookID3D12CommandListResourceBarrier(ID3D12CommandList *list, UINT NumBarriers, const D3D12_RESOURCE_BARRIER *pBarriers);
void WINAPI HookID3D12CommandListBarrier(ID3D12CommandList *list, UINT NumBarrierGroups, const D3D12_BARRIER_GROUP *pBarrierGroups);
void WINAPI HookID3D12CommandListBeginRenderPass(ID3D12CommandList* list, UINT NumRenderTargets, const D3D12_RENDER_PASS_RENDER_TARGET_DESC* pRenderTargets, const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* pDepthStencil, D3D12_RENDER_PASS_FLAGS Flags);
void WINAPI HookID3D12CommandListEndRenderPass(ID3D12CommandList* list);
void WINAPI HookID3D12CommandListDrawInstanced(ID3D12CommandList *list, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
void WINAPI HookID3D12CommandListDrawIndexedInstanced(ID3D12CommandList *list, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
void WINAPI HookID3D12CommandListDispatch(ID3D12CommandList *list, UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);
void WINAPI HookID3D12CommandListExecuteIndirect(ID3D12CommandList *list, ID3D12CommandSignature *pCommandSignature, UINT MaxCommandCount, ID3D12Resource *pArgumentBuffer, UINT64 ArgumentBufferOffset, ID3D12Resource *pCountBuffer, UINT64 CountBufferOffset);
void WINAPI HookID3D12CommandListCopyTiles(ID3D12CommandList* list, ID3D12Resource* pTiledResource, const D3D12_TILED_RESOURCE_COORDINATE* pTileRegionStartCoordinate, const D3D12_TILE_REGION_SIZE* pTileRegionSize, ID3D12Resource* pBuffer, UINT64 BufferStartOffsetInBytes, D3D12_TILE_COPY_FLAGS Flags);
void WINAPI HookID3D12CommandListDispatchMesh(ID3D12CommandList* list, UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);
void WINAPI HookID3D12CommandListSetComputeRoot32BitConstant(ID3D12CommandList *list, UINT RootParameterIndex, UINT SrcData, UINT DestOffsetIn32BitValues);
void WINAPI HookID3D12CommandListSetGraphicsRoot32BitConstant(ID3D12CommandList *list, UINT RootParameterIndex, UINT SrcData, UINT DestOffsetIn32BitValues);
void WINAPI HookID3D12CommandListSetComputeRoot32BitConstants(ID3D12CommandList *list, UINT RootParameterIndex, UINT Num32BitValuesToSet, const void *pSrcData, UINT DestOffsetIn32BitValues);
void WINAPI HookID3D12CommandListSetGraphicsRoot32BitConstants(ID3D12CommandList *list, UINT RootParameterIndex, UINT Num32BitValuesToSet, const void *pSrcData, UINT DestOffsetIn32BitValues);
HRESULT WINAPI HookID3D12CommandListGetDevice(ID3D12CommandList *_this, REFIID riid, void **ppDevice);
D3D12_COMMAND_LIST_TYPE WINAPI HookID3D12CommandListGetType(ID3D12CommandList *_this);

/// Extension hooks
DX12_C_LINKAGE AGSReturnCode HookAMDAGSDestroyDevice(AGSContext *context, ID3D12Device *device, unsigned int *deviceReferences);
DX12_C_LINKAGE AGSReturnCode HookAMDAGSPushMarker(AGSContext *context, ID3D12GraphicsCommandList *commandList, const char *data);
DX12_C_LINKAGE AGSReturnCode HookAMDAGSPopMarker(AGSContext *context, ID3D12GraphicsCommandList *commandList);
DX12_C_LINKAGE AGSReturnCode HookAMDAGSSetMarker(AGSContext *context, ID3D12GraphicsCommandList *commandList, const char *data);
