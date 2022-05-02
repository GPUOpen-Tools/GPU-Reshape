#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateRootSignature(ID3D12Device*, UINT, const void*, SIZE_T, const IID* const, void**);
ULONG WINAPI HookID3D12RootSignatureRelease(ID3D12RootSignature* signature);
