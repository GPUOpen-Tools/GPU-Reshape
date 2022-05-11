#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateCommandQueue(ID3D12Device*, const D3D12_COMMAND_QUEUE_DESC*, const IID&, void**);
HRESULT WINAPI HookID3D12DeviceCreateCommandAllocator(ID3D12Device*, D3D12_COMMAND_LIST_TYPE, const IID&, void**);
HRESULT WINAPI HookID3D12DeviceCreateCommandList(ID3D12Device*, UINT, D3D12_COMMAND_LIST_TYPE, ID3D12CommandAllocator*, ID3D12PipelineState*, const IID&, void**);
void WINAPI HookID3D12CommandQueueExecuteCommandLists(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);
ULONG WINAPI HookID3D12CommandQueueRelease(ID3D12CommandQueue* queue);
ULONG WINAPI HookID3D12CommandAllocatorRelease(ID3D12CommandAllocator* allocator);
HRESULT WINAPI HookID3D12CommandListReset(ID3D12CommandList* list, ID3D12CommandAllocator* allocator, ID3D12PipelineState* state);
HRESULT WINAPI HookID3D12CommandListClose(ID3D12CommandList* list);
void WINAPI HookID3D12CommandListSetPipelineState(ID3D12CommandList* list, ID3D12PipelineState* pipeline);
ULONG WINAPI HookID3D12CommandListRelease(ID3D12CommandList* list);
