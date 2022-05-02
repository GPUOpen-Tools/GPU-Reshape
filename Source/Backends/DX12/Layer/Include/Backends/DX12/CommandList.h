#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateCommandQueue(ID3D12Device*, const D3D12_COMMAND_QUEUE_DESC*, const IID* const, void**);
HRESULT WINAPI HookID3D12DeviceCreateCommandAllocator(ID3D12Device*, D3D12_COMMAND_LIST_TYPE, const IID* const, void**);
HRESULT WINAPI HookID3D12DeviceCreateCommandList(ID3D12Device*, UINT, D3D12_COMMAND_LIST_TYPE, ID3D12CommandAllocator*, ID3D12PipelineState*, const IID* const, void**);
void WINAPI HookID3D12CommandQueueExecuteCommandLists(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);
ULONG WINAPI HookID3D12CommandQueueRelease(ID3D12CommandQueue* queue);
ULONG WINAPI HookID3D12CommandAllocatorRelease(ID3D12CommandAllocator* allocator);
ULONG WINAPI HookID3D12CommandListRelease(ID3D12CommandList* list);
ULONG WINAPI HookID3D12GraphicsCommandListRelease(ID3D12GraphicsCommandList* list);
