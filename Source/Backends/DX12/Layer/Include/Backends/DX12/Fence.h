#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateFence(ID3D12Device*, UINT64, D3D12_FENCE_FLAGS, const IID&, void**);
HRESULT WINAPI HookID3D12CommandQueueSignal(ID3D12CommandQueue* _this, ID3D12Fence* pFence, UINT64 Value);
