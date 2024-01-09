// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
#include <Backends/DX12/States/DXGIOutputState.h>
#include <Backends/DX12/States/DXGIAdapterState.h>
#include <Backends/DX12/States/DXGIOutputDuplicationState.h>

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

HRESULT WINAPI HookIDXGIFactoryEnumAdapters(IDXGIFactory* self, UINT Output, IDXGIAdapter **ppAdapter) {
    auto table = GetTable(self);

    // Object
    IDXGIAdapter *adapter{nullptr};

    // Pass down callchain
    HRESULT hr = table.next->EnumAdapters(Output, &adapter);
    if (FAILED(hr)) {
        return hr;
    }

    // Default allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDXGIFactory) DXGIAdapterState();
    state->allocators = allocators;
    state->parent = self;

    // Keep reference to parent
    self->AddRef();

    // Create detours
    adapter = CreateDetour(state->allocators, adapter, state);

    // Query to external object if requested
    if (ppAdapter) {
        *ppAdapter = adapter;
    } else {
        adapter->Release();
    }

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGIFactoryEnumAdapters1(IDXGIFactory* self, UINT Output, IDXGIAdapter1 **ppAdapter) {
    auto table = GetTable(self);

    // Object
    IDXGIAdapter1 *adapter{nullptr};

    // Pass down callchain
    HRESULT hr = table.next->EnumAdapters1(Output, &adapter);
    if (FAILED(hr)) {
        return hr;
    }

    // Default allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDXGIFactory) DXGIAdapterState();
    state->allocators = allocators;
    state->parent = self;

    // Keep reference to parent
    self->AddRef();

    // Create detours
    adapter = static_cast<IDXGIAdapter1*>(CreateDetour(state->allocators, adapter, state));

    // Query to external object if requested
    if (ppAdapter) {
        *ppAdapter = adapter;
    } else {
        adapter->Release();
    }

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGIFactoryEnumAdapterByLuid(IDXGIFactory* self, LUID AdapterLuid, const IID& riid, void** ppvAdapter) {
    auto table = GetTable(self);

    // Object
    IDXGIAdapter *adapter{nullptr};

    // Pass down callchain
    HRESULT hr = table.next->EnumAdapterByLuid(AdapterLuid, __uuidof(IDXGIAdapter), reinterpret_cast<void**>(&adapter));
    if (FAILED(hr)) {
        return hr;
    }

    // Default allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDXGIFactory) DXGIAdapterState();
    state->allocators = allocators;
    state->parent = self;

    // Keep reference to parent
    self->AddRef();

    // Create detours
    adapter = CreateDetour(state->allocators, adapter, state);

    // Query to external object if requested
    if (ppvAdapter) {
        hr = adapter->QueryInterface(riid, ppvAdapter);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    adapter->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGIFactoryEnumWarpAdapter(IDXGIFactory* self, const IID& riid, void** ppvAdapter) {
    auto table = GetTable(self);

    // Object
    IDXGIAdapter *adapter{nullptr};

    // Pass down callchain
    HRESULT hr = table.next->EnumWarpAdapter(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&adapter));
    if (FAILED(hr)) {
        return hr;
    }

    // Default allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDXGIFactory) DXGIAdapterState();
    state->allocators = allocators;
    state->parent = self;

    // Keep reference to parent
    self->AddRef();

    // Create detours
    adapter = CreateDetour(state->allocators, adapter, state);

    // Query to external object if requested
    if (ppvAdapter) {
        hr = adapter->QueryInterface(riid, ppvAdapter);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    adapter->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGIFactoryEnumAdapterByGpuPreference(IDXGIFactory* self, UINT Adapter, DXGI_GPU_PREFERENCE GpuPreference, const IID& riid, void** ppvAdapter) {
    auto table = GetTable(self);

    // Object
    IDXGIAdapter *adapter{nullptr};

    // Pass down callchain
    HRESULT hr = table.next->EnumAdapterByGpuPreference(Adapter, GpuPreference, __uuidof(IDXGIAdapter), reinterpret_cast<void**>(&adapter));
    if (FAILED(hr)) {
        return hr;
    }

    // Default allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDXGIFactory) DXGIAdapterState();
    state->allocators = allocators;
    state->parent = self;

    // Keep reference to parent
    self->AddRef();

    // Create detours
    adapter = CreateDetour(state->allocators, adapter, state);

    // Query to external object if requested
    if (ppvAdapter) {
        hr = adapter->QueryInterface(riid, ppvAdapter);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    adapter->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGIAdapterEnumOutputs(IDXGIAdapter* self, UINT Output, IDXGIOutput **ppOutput) {
    auto table = GetTable(self);

    // Object
    IDXGIOutput *adapter{nullptr};

    // Pass down callchain
    HRESULT hr = table.next->EnumOutputs(Output, &adapter);
    if (FAILED(hr)) {
        return hr;
    }

    // Default allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDXGIFactory) DXGIOutputState();
    state->allocators = allocators;
    state->parent = self;

    // Create detours
    adapter = CreateDetour(state->allocators, adapter, state);

    // Query to external object if requested
    if (ppOutput) {
        *ppOutput = adapter;
    } else {
        adapter->Release();
    }

    // OK
    return S_OK;
}

HRESULT WINAPI HookIDXGIOutputDuplicateOutput(IDXGIOutput* self, IUnknown *pDevice, IDXGIOutputDuplication **ppOutputDuplication) {
    auto table = GetTable(self);

    // Object
    IDXGIOutputDuplication *duplicateOutput{nullptr};

    // Pass down callchain
    HRESULT hr = table.next->DuplicateOutput(pDevice, &duplicateOutput);
    if (FAILED(hr)) {
        return hr;
    }

    // Default allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDXGIFactory) DXGIOutputDuplicationState();
    state->allocators = allocators;

    // Create detours
    duplicateOutput = CreateDetour(state->allocators, duplicateOutput, state);

    // Query to external object if requested
    if (ppOutputDuplication) {
        *ppOutputDuplication = duplicateOutput;
    } else {
        duplicateOutput->Release();
    }

    // OK
    return S_OK;
}

HRESULT HookIDXGIFactoryGetParent(IDXGIFactory *_this, const IID &riid, void **ppParent) {
    auto table = GetTable(_this);

    // Pass to next query
    // TODO: Exactly what does a parent of a factory imply?
    return table.next->QueryInterface(riid, ppParent);
}

HRESULT HookIDXGIAdapterGetParent(IDXGIAdapter *_this, const IID &riid, void **ppParent) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppParent);
}

HRESULT WINAPI HookIDXGIOutputGetParent(IDXGIOutput* _this, REFIID riid, void **ppParent) {
    auto table = GetTable(_this);
    
    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppParent);
}

DXGIFactoryState::~DXGIFactoryState() {

}

DXGIAdapterState::~DXGIAdapterState() {
    parent->Release();
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
