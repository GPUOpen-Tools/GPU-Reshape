// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#include <Backends/DX12/Allocation/DeviceAllocator.h>

DeviceAllocator::~DeviceAllocator() {
    if (wcHostPool) {
        wcHostPool->Release();
    }

    if (allocator) {
        allocator->Release();
    }
}

bool DeviceAllocator::Install(ID3D12Device* device, IDXGIAdapter* adapter) {
    // Attempt to create allocator
    D3D12MA::ALLOCATOR_DESC desc{};
    desc.pDevice = device;
    desc.pAdapter = adapter;
    desc.Flags |= D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
    if (FAILED(D3D12MA::CreateAllocator(&desc, &allocator))) {
        return false;
    }

    // Attempt to create special host pool
    D3D12MA::POOL_DESC wcHostPoolDesc{};
    wcHostPoolDesc.Flags = D3D12MA::POOL_FLAG_NONE;
    wcHostPoolDesc.HeapFlags = D3D12_HEAP_FLAG_NONE;
    wcHostPoolDesc.HeapProperties.CreationNodeMask = 1u;
    wcHostPoolDesc.HeapProperties.VisibleNodeMask = 1u;
    wcHostPoolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
    wcHostPoolDesc.HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
    wcHostPoolDesc.HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
    if (FAILED(allocator->CreatePool(&wcHostPoolDesc, &wcHostPool))) {
        return false;
    }

    // OK
    return true;
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
        case AllocationResidency::Device: {
            allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            state = D3D12_RESOURCE_STATE_COMMON;
            break;
        }
        case AllocationResidency::Host:
        case AllocationResidency::HostVisible: {
            allocDesc.HeapType = D3D12_HEAP_TYPE_CUSTOM;
            state = D3D12_RESOURCE_STATE_COPY_DEST;

            // Set to special write-combine pool
            allocDesc.CustomPool = wcHostPool;
            break;
        }
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
        case AllocationResidency::HostVisible: {
            allocation.device = Allocate(desc, AllocationResidency::HostVisible);
            allocation.host = allocation.device;
            break;
        }
    }

    return allocation;
}

void DeviceAllocator::Free(const Allocation& allocation) {
    allocation.allocation->Release();
    allocation.resource->Release();
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

void DeviceAllocator::FlushMappedRange(const Allocation &allocation, uint64_t offset, uint64_t length) {
    // None needed
}
