#pragma once

// Backend
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/Layer.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateCommandQueue(ID3D12Device*, const D3D12_COMMAND_QUEUE_DESC*, const IID&, void**);
HRESULT WINAPI HookID3D12DeviceCreateCommandAllocator(ID3D12Device*, D3D12_COMMAND_LIST_TYPE, const IID&, void**);
HRESULT WINAPI HookID3D12DeviceCreateCommandList(ID3D12Device*, UINT, D3D12_COMMAND_LIST_TYPE, ID3D12CommandAllocator*, ID3D12PipelineState*, const IID&, void**);
void WINAPI HookID3D12CommandQueueExecuteCommandLists(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);
ULONG WINAPI HookID3D12CommandQueueRelease(ID3D12CommandQueue* queue);
HRESULT WINAPI HookID3D12CommandQueueSignal(ID3D12CommandQueue* _this, ID3D12Fence* pFence, UINT64 Value);
HRESULT WINAPI HookID3D12CommandQueueWait(ID3D12CommandQueue* _this, ID3D12Fence* pFence, UINT64 Value);
ULONG WINAPI HookID3D12CommandAllocatorRelease(ID3D12CommandAllocator* allocator);
HRESULT WINAPI HookID3D12CommandListReset(ID3D12CommandList* list, ID3D12CommandAllocator* allocator, ID3D12PipelineState* state);
HRESULT WINAPI HookID3D12CommandListClose(ID3D12CommandList* list);
void WINAPI HookID3D12CommandListSetPipelineState(ID3D12CommandList* list, ID3D12PipelineState* pipeline);
void WINAPI HookID3D12CommandListSetGraphicsRootSignature(ID3D12CommandList* list, ID3D12RootSignature* rootSignature);
void WINAPI HookID3D12CommandListSetComputeRootSignature(ID3D12CommandList* list, ID3D12RootSignature* rootSignature);
void WINAPI HookID3D12CommandListSetDescriptorHeaps(ID3D12CommandList* list, UINT NumDescriptorHeaps, ID3D12DescriptorHeap *const *ppDescriptorHeaps);
void WINAPI HookID3D12CommandListSetDescriptorHeaps(ID3D12CommandList* list, UINT NumDescriptorHeaps, ID3D12DescriptorHeap *const *ppDescriptorHeaps);
void WINAPI HookID3D12CommandListCopyTextureRegion(ID3D12CommandList* list, const D3D12_TEXTURE_COPY_LOCATION* pDst, UINT DstX, UINT DstY, UINT DstZ, const D3D12_TEXTURE_COPY_LOCATION* pSrc, const D3D12_BOX* pSrcBox);
void WINAPI HookID3D12CommandListResourceBarrier(ID3D12CommandList* list, UINT NumBarriers, const D3D12_RESOURCE_BARRIER* pBarriers);
ULONG WINAPI HookID3D12CommandListRelease(ID3D12CommandList* list);

/// Extension hooks
DX12_C_LINKAGE AGSReturnCode HookAMDAGSDestroyDevice(AGSContext* context, ID3D12Device* device, unsigned int* deviceReferences);
DX12_C_LINKAGE AGSReturnCode HookAMDAGSPushMarker(AGSContext* context, ID3D12GraphicsCommandList* commandList, const char* data);
DX12_C_LINKAGE AGSReturnCode HookAMDAGSPopMarker(AGSContext* context, ID3D12GraphicsCommandList* commandList);
DX12_C_LINKAGE AGSReturnCode HookAMDAGSSetMarker(AGSContext* context, ID3D12GraphicsCommandList* commandList, const char* data);
