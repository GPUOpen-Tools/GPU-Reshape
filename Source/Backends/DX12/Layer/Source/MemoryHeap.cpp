#include <Backends/DX12/MemoryHeap.h>
#include <Backends/DX12/Table.Gen.h>

HRESULT WINAPI HookID3D12DeviceCreateHeap(ID3D12Device* device, const D3D12_HEAP_DESC *pDesc, REFIID riid, void **ppvHeap) {
    auto table = GetTable(device);

    // Object
    ID3D12Heap* heap{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateHeap(table.next, pDesc, __uuidof(ID3D12Heap), reinterpret_cast<void**>(&heap));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto* state = new (table.state->allocators, kAllocStateFence) MemoryHeapState();
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

HRESULT WINAPI HookID3D12DeviceCreateHeap1(ID3D12Device* device, const D3D12_HEAP_DESC *pDesc, ID3D12ProtectedResourceSession *pProtectedSession, REFIID riid, void **ppvHeap) {
    auto table = GetTable(device);

    // Object
    ID3D12Heap* heap{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateHeap1(table.next, pDesc, pProtectedSession, __uuidof(ID3D12Heap), reinterpret_cast<void**>(&heap));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto* state = new (table.state->allocators, kAllocStateFence) MemoryHeapState();
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

HRESULT WINAPI HookID3D12DeviceOpenExistingHeapFromAddress(ID3D12Device3* device, const void* pAddress, const IID& riid, void** ppvHeap) {
    auto table = GetTable(device);

    // Object
    ID3D12Heap* heap{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_OpenExistingHeapFromAddress(table.next, pAddress, __uuidof(ID3D12Heap), reinterpret_cast<void**>(&heap));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto* state = new (table.state->allocators, kAllocStateFence) MemoryHeapState();
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

HRESULT WINAPI HookID3D12DeviceOpenExistingHeapFromFileMapping(ID3D12Device3* device, HANDLE hFileMapping, const IID& riid, void** ppvHeap) {
    auto table = GetTable(device);

    // Object
    ID3D12Heap* heap{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_OpenExistingHeapFromFileMapping(table.next, hFileMapping, __uuidof(ID3D12Heap), reinterpret_cast<void**>(&heap));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto* state = new (table.state->allocators, kAllocStateFence) MemoryHeapState();
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

HRESULT WINAPI HookID3D12HeapGetDevice(ID3D12Heap* _this, REFIID riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}
