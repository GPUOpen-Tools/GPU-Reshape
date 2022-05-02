#pragma once

// Backend
#include <Backends/DX12/DX12.h>

struct GlobalDeviceDetour {
public:
    bool Install();
    void Uninstall();
};

/// Hooks
HRESULT WINAPI HookID3D12CreateDedvice(_In_opt_ IUnknown *pAdapter, D3D_FEATURE_LEVEL minimumFeatureLevel, _In_ REFIID riid, _COM_Outptr_opt_ void **ppDevice);
ULONG WINAPI HookID3D12DeviceRelease(ID3D12Device* device);
