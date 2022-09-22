#include <Backends/DX12/Resource.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/ResourceState.h>
#include <Backends/DX12/States/DeviceState.h>

HRESULT HookID3D12ResourceMap(ID3D12Resource* resource, UINT subresource, const D3D12_RANGE* readRange, void** blob) {
    auto table = GetTable(resource);

    // Pass down callchain
    HRESULT hr = table.bottom->next_Map(table.next, subresource, readRange, blob);
    if (FAILED(hr)) {
        return hr;
    }

    // OK
    return S_OK;
}

static ID3D12Resource* CreateResourceState(const DeviceTable& table, ID3D12Resource* resource) {
    // Create state
    auto* state = new ResourceState();
    state->allocators = table.state->allocators;
    state->parent = table.state;

    // Create detours
    return CreateDetour(Allocators{}, resource, state);
}

HRESULT HookID3D12DeviceCreateCommittedResource(ID3D12Device* device, const D3D12_HEAP_PROPERTIES* heap, D3D12_HEAP_FLAGS heapFlags, const D3D12_RESOURCE_DESC* desc, D3D12_RESOURCE_STATES resourceState, const D3D12_CLEAR_VALUE* clearValue, const IID& riid, void** pResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateCommittedResource(table.next, heap, heapFlags, desc, resourceState, clearValue, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    resource = CreateResourceState(table, resource);

    // Query to external object if requested
    if (pResource) {
        hr = resource->QueryInterface(riid, pResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreatePlacedResource(ID3D12Device *device, ID3D12Heap * heap, UINT64 heapFlags, const D3D12_RESOURCE_DESC *desc, D3D12_RESOURCE_STATES resourceState, const D3D12_CLEAR_VALUE * clearValue, const IID& riid, void ** pResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreatePlacedResource(table.next, heap, heapFlags, desc, resourceState, clearValue, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    resource = CreateResourceState(table, resource);

    // Query to external object if requested
    if (pResource) {
        hr = resource->QueryInterface(riid, pResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateReservedResource(ID3D12Device *device, const D3D12_RESOURCE_DESC *desc, D3D12_RESOURCE_STATES resourceState, const D3D12_CLEAR_VALUE *clearValue, const IID& riid, void **pResource) {
    auto table = GetTable(device);

    // Object
    ID3D12Resource* resource{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateReservedResource(table.next, desc, resourceState, clearValue, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&resource));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    resource = CreateResourceState(table, resource);

    // Query to external object if requested
    if (pResource) {
        hr = resource->QueryInterface(riid, pResource);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    resource->Release();

    // OK
    return S_OK;
}

ResourceState::~ResourceState() {

}
