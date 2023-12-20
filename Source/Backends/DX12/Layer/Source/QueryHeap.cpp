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

#include <Backends/DX12/QueryHeap.h>
#include <Backends/DX12/Table.Gen.h>

HRESULT WINAPI HookID3D12DeviceCreateQueryHeap(ID3D12Device* device, const D3D12_QUERY_HEAP_DESC *pDesc, REFIID riid, void **ppvHeap) {
    auto table = GetTable(device);

    // Object
    ID3D12QueryHeap* heap{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateQueryHeap(table.next, pDesc, __uuidof(ID3D12QueryHeap), reinterpret_cast<void**>(&heap));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto* state = new (table.state->allocators, kAllocStateFence) QueryHeapState();
    state->allocators = table.state->allocators;
    state->parent = device;

    // Create detours
    heap = CreateDetour(state->allocators, heap, state);

    // Query to external object if requested
    if (ppvHeap) {
        hr = heap->QueryInterface(riid, ppvHeap);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    heap->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12QueryHeapGetDevice(ID3D12QueryHeap* _this, REFIID riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}
