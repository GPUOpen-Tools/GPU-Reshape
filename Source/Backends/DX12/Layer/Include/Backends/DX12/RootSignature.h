#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreateRootSignature(ID3D12Device*, UINT, const void*, SIZE_T, const IID&, void**);
HRESULT WINAPI HookID3D12RootSignatureGetDevice(ID3D12RootSignature* _this, REFIID riid, void **ppDevice);
