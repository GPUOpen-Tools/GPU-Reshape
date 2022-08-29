#include <Backends/DX12/Allocation/DeviceAllocator.h>

DeviceAllocator::~DeviceAllocator() {
    if (allocator) {
        allocator->Release();
    }
}

bool DeviceAllocator::Install(ID3D12Device* device, IDXGIAdapter* adapter) {
    D3D12MA::ALLOCATOR_DESC desc{};
    desc.pDevice = device;
    desc.pAdapter = adapter;
    desc.Flags |= D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;

    // Attempt to create
    HRESULT hr = D3D12MA::CreateAllocator(&desc, &allocator);
    return SUCCEEDED(hr);
}

Allocation DeviceAllocator::Allocate(const D3D12_RESOURCE_DESC& desc, AllocationResidency residency) {
    Allocation allocation{};

    // Region info
    D3D12MA::ALLOCATION_DESC allocDesc = {};

    // Initial state
    D3D12_RESOURCE_STATES state;

    // Translate residency
    D3D12_RESOURCE_DESC filteredDesc = desc;
    switch (residency) {
        case AllocationResidency::Device:
            allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            state = D3D12_RESOURCE_STATE_COMMON;
            break;
        case AllocationResidency::Host:
            allocDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
            state = D3D12_RESOURCE_STATE_COPY_DEST;

            filteredDesc.Flags &= ~D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            break;
    }

    // Attempt to allocate the resource
    HRESULT hr = allocator->CreateResource(&allocDesc, &filteredDesc, state, nullptr, &allocation.allocation, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&allocation.resource));
    if (FAILED(hr)) {
        return {};
    }

    return allocation;
}

MirrorAllocation DeviceAllocator::AllocateMirror(const D3D12_RESOURCE_DESC& desc, AllocationResidency residency) {
    MirrorAllocation allocation;

    switch (residency) {
        case AllocationResidency::Device: {
            allocation.device = Allocate(desc, AllocationResidency::Device);
            allocation.host = Allocate(desc, AllocationResidency::Host);
            break;
        }
        case AllocationResidency::Host: {
            allocation.device = Allocate(desc, AllocationResidency::Host);
            allocation.host = allocation.device;
            break; 
        }
    }

    return allocation;
}

void DeviceAllocator::Free(const Allocation& allocation) {
    allocation.resource->Release();
    allocation.allocation->Release();
}

void DeviceAllocator::Free(const MirrorAllocation& mirrorAllocation) {
    if (mirrorAllocation.host.allocation != mirrorAllocation.device.allocation) {
        Free(mirrorAllocation.host);
    }

    Free(mirrorAllocation.device);
}

void *DeviceAllocator::Map(const Allocation &allocation) {
    void* data{nullptr};

    HRESULT hr = allocation.resource->Map(0, nullptr, &data);
    if (FAILED(hr)) {
        return nullptr;
    }

    return data;
}

void DeviceAllocator::Unmap(const Allocation &allocation) {
    allocation.resource->Unmap(0, nullptr);
}
