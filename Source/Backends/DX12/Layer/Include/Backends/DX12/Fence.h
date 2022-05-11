#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateFence(ID3D12Device*, UINT64, D3D12_FENCE_FLAGS, const IID&, void**);
ULONG WINAPI HookID3D12FenceRelease(ID3D12Fence* fence);
