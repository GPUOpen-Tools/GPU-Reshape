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
