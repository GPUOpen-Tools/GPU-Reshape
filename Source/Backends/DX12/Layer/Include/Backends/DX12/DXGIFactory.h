#pragma once

// Backend
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/Layer.h>

struct GlobalDXGIFactoryDetour {
public:
    bool Install();
    void Uninstall();
};

/// Hooks
DX12_C_LINKAGE HRESULT WINAPI HookCreateDXGIFactory(REFIID riid, _COM_Outptr_ void **ppFactory);
DX12_C_LINKAGE HRESULT WINAPI HookCreateDXGIFactory1(REFIID riid, _COM_Outptr_ void **ppFactory);
DX12_C_LINKAGE HRESULT WINAPI HookCreateDXGIFactory2(UINT flags, REFIID riid, _COM_Outptr_ void **ppFactory);
ULONG WINAPI HookIDXGIFactoryRelease(IDXGIFactory* factory);
