#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateFence(ID3D12Device*, UINT64, D3D12_FENCE_FLAGS, const IID&, void**);
HRESULT WINAPI HookID3D12DeviceSetEventOnMultipleFenceCompletion(ID3D12Device* _this, ID3D12Fence* const* ppFences, const UINT64* pFenceValues, UINT NumFences, D3D12_MULTIPLE_FENCE_WAIT_FLAGS Flags, HANDLE hEvent);
HRESULT WINAPI HookID3D12CommandQueueSignal(ID3D12CommandQueue* _this, ID3D12Fence* pFence, UINT64 Value);
HRESULT WINAPI HookID3D12FenceGetDevice(ID3D12Fence* _this, REFIID riid, void **ppDevice);
