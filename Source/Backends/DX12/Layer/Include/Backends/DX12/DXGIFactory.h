#pragma once

// Backend
#include <Backends/DX12/DX12.h>

struct GlobalDXGIFactoryDetour {
public:
    bool Install();
    void Uninstall();
};

/// Hooks
HRESULT WINAPI HookCreateDXGIFactory(REFIID riid, _COM_Outptr_ void **ppFactory);
HRESULT WINAPI HookCreateDXGIFactory1(REFIID riid, _COM_Outptr_ void **ppFactory);
HRESULT WINAPI HookCreateDXGIFactory2(UINT flags, REFIID riid, _COM_Outptr_ void **ppFactory);
ULONG WINAPI HookIDXGIFactoryRelease(IDXGIFactory* factory);
