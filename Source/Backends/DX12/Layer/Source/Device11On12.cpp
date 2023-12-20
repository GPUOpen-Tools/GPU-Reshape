// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

// Layer
#include <Backends/DX12/Device.h>
#include <Backends/DX12/Resource.h>
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/Device11State.h>
#include <Backends/DX12/States/Device11On12State.h>

HRESULT HookD3D11On12CreateDevice(
    IUnknown *pDevice,
    UINT Flags,
    const D3D_FEATURE_LEVEL *pFeatureLevels,
    UINT FeatureLevels,
    IUnknown *const *ppCommandQueues,
    UINT NumQueues,
    UINT NodeMask,
    ID3D11Device **ppDevice,
    ID3D11DeviceContext **ppImmediateContext,
    D3D_FEATURE_LEVEL *pChosenFeatureLevel
    ) {
    // Unwrap the queues
    auto* unwrappedQueues = ALLOCA_ARRAY(IUnknown*, NumQueues);
    for (uint32_t i = 0; i < NumQueues; i++) {
        unwrappedQueues[i] = UnwrapObject(ppCommandQueues[i]);
    }

    // Next device
    ID3D11Device* device11{nullptr};

    // Pass down callchain
    HRESULT hr = D3D12GPUOpenFunctionTableNext.next_D3D11On12CreateDeviceOriginal(
        UnwrapObject(pDevice),
        Flags,
        pFeatureLevels,
        FeatureLevels,
        unwrappedQueues,
        NumQueues,
        NodeMask,
        &device11,
        ppImmediateContext,
        pChosenFeatureLevel
    );
    if (FAILED(hr)) {
        return hr;
    }

    // Allocators
    Allocators allocators = {};

    // Create state
    auto *state = new (allocators, kAllocStateDevice) Device11State(allocators);
    state->object = device11;

    // Create shared 11On12 wrapper
    {
        // Create state
        auto *wrapperState = new (allocators, kAllocStateDevice) Device11On12State(allocators);

        // Query underlying 11 on 12, must exist
        if (FAILED(device11->QueryInterface(__uuidof(ID3D11On12Device), reinterpret_cast<void**>(&wrapperState->object)))) {
            return false;
        }
    
        // Create detours
        state->wrapped11On12 = CreateDetour(allocators, wrapperState->object, wrapperState);
    }

    // Create detours
    device11 = CreateDetour(allocators, state->object, state);
    
    // Query to external object if requested
    if (ppDevice) {
        *ppDevice = device11;
        device11->AddRef();
    }
    
    // Cleanup
    device11->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D11DeviceQueryInterface(ID3D11Device* device, const IID& riid, void** ppvObject) {
    auto table = GetTable(device);

    // Querying wrapper?
    if (riid == __uuidof(ID3D11On12Device)) {
        device->AddRef();
        *ppvObject = table.state->wrapped11On12;
        return S_OK;
    }

    if (riid == __uuidof(Device11State)) {
        /* No ref added */
        *ppvObject = table.state;
        return S_OK;
    }

    if (riid == IID_Unwrap) {
        /* No ref added */
        *ppvObject = table.next;
        return S_OK;
    }

    if (riid == __uuidof(ID3D11Device)) {
        device->AddRef();
        *ppvObject = static_cast<ID3D11Device*>(device);
        return S_OK;
    } else if (riid == __uuidof(IUnknown)) {
        device->AddRef();
        *ppvObject = static_cast<IUnknown*>(device);
        return S_OK;
    }

    return table.next->QueryInterface(riid, ppvObject);
}

void WINAPI HookID3D11On12DeviceReleaseWrappedResources(ID3D11On12Device* device, ID3D11Resource* const* ppResources, UINT NumResources) {
    auto table = GetTable(device);
    
    // Unwrap the resources
    auto* unwrapped = ALLOCA_ARRAY(ID3D11Resource*, NumResources);
    for (uint32_t i = 0; i < NumResources; i++) {
        unwrapped[i] = UnwrapObject(ppResources[i]);
    }

    // Pass down callchain
    return table.next->ReleaseWrappedResources(unwrapped, NumResources);
}

void WINAPI HookID3D11On12DeviceAcquireWrappedResources(ID3D11On12Device* device, ID3D11Resource* const* ppResources, UINT NumResources) {
    auto table = GetTable(device);
    
    // Unwrap the resources
    auto* unwrapped = ALLOCA_ARRAY(ID3D11Resource*, NumResources);
    for (uint32_t i = 0; i < NumResources; i++) {
        unwrapped[i] = UnwrapObject(ppResources[i]);
    }

    // Pass down callchain
    return table.next->AcquireWrappedResources(unwrapped, NumResources);
}

HRESULT WINAPI HookID3D11On12DeviceReturnUnderlyingResource(ID3D11On12Device* device, ID3D11Resource* pResource11, UINT NumSync, UINT64* pSignalValues, ID3D12Fence** ppFences) {
    auto table = GetTable(device);
    
    // Unwrap the fences
    auto* unwrapped = ALLOCA_ARRAY(ID3D12Fence*, NumSync);
    for (uint32_t i = 0; i < NumSync; i++) {
        unwrapped[i] = UnwrapObject(ppFences[i]);
    }

    // Pass down callchain
    return table.next->ReturnUnderlyingResource(pResource11, NumSync, pSignalValues, unwrapped);
}

Device11On12State::~Device11On12State() {
    // Special case, release internal interface
    object->Release();
}

Device11State::~Device11State() {
    // Release shared wrapper
    if (wrapped11On12) {
        wrapped11On12->Release();
    }
}
