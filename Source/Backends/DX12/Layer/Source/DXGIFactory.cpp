// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#include <Backends/DX12/DXGIFactory.h>
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/DXGIFactoryState.h>

// Detour
#include <Detour/detours.h>

HRESULT WINAPI HookCreateDXGIFactory(REFIID riid, _COM_Outptr_ void **ppFactory) {
    // Object
    IDXGIFactory *factory{nullptr};

    // Pass down callchain
    HRESULT hr = D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactoryOriginal(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return hr;
    }

    // Default allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDXGIFactory) DXGIFactoryState();
    state->allocators = allocators;

    // Create detours
    factory = CreateDetour(state->allocators, factory, state);

    // Query to external object if requested
    if (ppFactory) {
        hr = factory->QueryInterface(riid, ppFactory);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    factory->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookCreateDXGIFactory1(REFIID riid, _COM_Outptr_ void **ppFactory) {
    // Object
    IDXGIFactory *factory{nullptr};

    // Pass down callchain
    HRESULT hr = D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory1Original(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return hr;
    }

    // Default allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDXGIFactory) DXGIFactoryState();
    state->allocators = allocators;
    
    // Create detours
    factory = CreateDetour(state->allocators, factory, state);

    // Query to external object if requested
    if (ppFactory) {
        hr = factory->QueryInterface(riid, ppFactory);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    factory->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookCreateDXGIFactory2(UINT flags, REFIID riid, _COM_Outptr_ void **ppFactory) {
    // Object
    IDXGIFactory *factory{nullptr};

    // Pass down callchain
    HRESULT hr = D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory2Original(flags, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return hr;
    }

    // Default allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDXGIFactory) DXGIFactoryState();
    state->allocators = allocators;

    // Create detours
    factory = CreateDetour(state->allocators, factory, state);

    // Query to external object if requested
    if (ppFactory) {
        hr = factory->QueryInterface(riid, ppFactory);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    factory->Release();

    // OK
    return S_OK;
}

DXGIFactoryState::~DXGIFactoryState() {

}

bool GlobalDXGIFactoryDetour::Install() {
    ASSERT(!D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactoryOriginal, "Global dxgi factory detour re-entry");

    // Attempt to find module
    HMODULE handle = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"dxgi.dll", &handle)) {
        return false;
    }

    // Attach against original address
    D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactoryOriginal = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>(GetProcAddress(handle, "CreateDXGIFactory"));
    DetourAttach(&reinterpret_cast<void*&>(D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactoryOriginal), reinterpret_cast<void*>(HookCreateDXGIFactory));

    // Attach against original address
    D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory1Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY1>(GetProcAddress(handle, "CreateDXGIFactory1"));
    if (D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory1Original) {
        DetourAttach(&reinterpret_cast<void*&>(D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory1Original), reinterpret_cast<void*>(HookCreateDXGIFactory1));
    }

    // Attach against original address
    D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory2Original = reinterpret_cast<PFN_CREATE_DXGI_FACTORY2>(GetProcAddress(handle, "CreateDXGIFactory2"));
    if (D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory2Original) {
        DetourAttach(&reinterpret_cast<void *&>(D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory2Original), reinterpret_cast<void *>(HookCreateDXGIFactory2));
    }

    // OK
    return true;
}

void GlobalDXGIFactoryDetour::Uninstall() {
    // Detach from detour
    DetourDetach(&reinterpret_cast<void*&>(D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactoryOriginal), reinterpret_cast<void*>(HookCreateDXGIFactory));
    if (D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory1Original) {
        DetourDetach(&reinterpret_cast<void*&>(D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory1Original), reinterpret_cast<void*>(HookCreateDXGIFactory1));
    }
    if (D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory2Original) {
        DetourDetach(&reinterpret_cast<void*&>(D3D12GPUOpenFunctionTableNext.next_CreateDXGIFactory2Original), reinterpret_cast<void*>(HookCreateDXGIFactory2));
    }
}
