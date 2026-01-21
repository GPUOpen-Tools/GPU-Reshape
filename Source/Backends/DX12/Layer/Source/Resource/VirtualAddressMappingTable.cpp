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

// Layer
#include <Backends/DX12/Resource/VirtualAddressMappingTable.h>
#include <Backends/DX12/Resource/VirtualAddressMappingTablePersistentVersion.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/States/DeviceState.h>

// Shared
#include <Shared/VirtualAddressMapping.h>

// Backend
#include <Backends/DX12/States/ResourceState.h>

VirtualAddressMappingTable::VirtualAddressMappingTable(DeviceState* device, const ComRef<DeviceAllocator> &deviceAllocator) : device(device), deviceAllocator(deviceAllocator) {

}

VirtualAddressMappingTable::~VirtualAddressMappingTable() {
    // Release all free versions
    for (VirtualAddressMappingTablePersistentVersion * version : freeVersions) {
        Destruct(version);
    }
}

VirtualAddressMappingTablePersistentVersion * VirtualAddressMappingTable::Allocate() {
    std::lock_guard guard(mutex);
    
    // Any free?
    if (!freeVersions.empty()) {
        VirtualAddressMappingTablePersistentVersion *version = freeVersions.back();
        freeVersions.pop_back();
        return version;
    }
    
    auto* version = new VirtualAddressMappingTablePersistentVersion();
    return version;
}

void VirtualAddressMappingTable::Free(VirtualAddressMappingTablePersistentVersion *version) {
    std::lock_guard guard(mutex);
    freeVersions.push_back(version);
}

void VirtualAddressMappingTable::Commit(VirtualAddressMappingTablePersistentVersion *version) {
    std::lock_guard guard(mutex);
    
    // Maintain lock hierarchy
    device->virtualAddressTable.Lock();

    // Required entry count
    uint64_t entryCount = device->virtualAddressTable.CountNoLock();
    
    // Requires re-allocation?
    if (!version->dwords || version->entryCount < entryCount) {
        if (version->allocation.device.resource) {
            deviceAllocator->Free(version->allocation);
        }
        
        // Safe mapping count
        version->entryCount = static_cast<uint32_t>(std::max(entryCount, 1ull) * 1.5f);
        version->dwordCount = VirtualAddressTableHeaderSizeDWord + VirtualAddressMappingDWordCount * version->entryCount;
    
        // Mapped description
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Alignment = 0;
        desc.Width = sizeof(uint32_t) * version->dwordCount;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.MipLevels = 1;
        desc.SampleDesc.Quality = 0;
        desc.SampleDesc.Count = 1;

        // Create allocation
        version->allocation = deviceAllocator->AllocateMirror(desc, AllocationResidency::HostUpload);

#ifndef NDEBUG
        version->allocation.device.resource->SetName(L"VAMTDevice");
        version->allocation.host.resource->SetName(L"VAMTHost");
#endif // NDEBUG

        // Opaque host data
        void* mappedOpaque{nullptr};

        // Map range
        D3D12_RANGE range;
        range.Begin = 0;
        range.End = sizeof(uint32_t) * version->dwordCount;
        version->allocation.host.resource->Map(0, &range, &mappedOpaque);

        // Store host
        version->dwords = static_cast<uint32_t*>(mappedOpaque);
        std::memset(version->dwords, 0, desc.Width);
    }
    
    // Write offset
    uint64_t dwordOffset = 0;
    
    // Write header
    version->dwords[VirtualAddressTableHeaderLengthDWord] = static_cast<uint32_t>(entryCount);
    dwordOffset += VirtualAddressTableHeaderSizeDWord;
    
    // Write all addresses
    for (auto&& [vaddr, entry] : device->virtualAddressTable.GetEntriesNoLock()) {
        uint32_t* mapping = version->dwords + dwordOffset;
        
        // Write header
        VirtualAddressMappingHeader* header = reinterpret_cast<VirtualAddressMappingHeader*>(mapping);
        header->VirtualAddress = vaddr;
        header->Length = entry.length;
        mapping += VirtualAddressMappingHeaderDWordCount;
        
        // Copy virtual mapping
        std::memcpy(mapping, &entry.state->virtualMapping, sizeof(uint32_t) * VirtualMappingDWordCount);
        static_assert(sizeof(uint32_t) * VirtualMappingDWordCount == sizeof(entry.state->virtualMapping), "Invalid mapping");
        
        dwordOffset += VirtualAddressMappingDWordCount;
    }
    
    // Unmap the written range
    // i.e. "done writing to this, flush!"
    D3D12_RANGE range;
    range.Begin = 0;
    range.End = sizeof(uint32_t) * version->dwordCount;
    version->allocation.host.resource->Unmap(0, &range);

    // Map contents
    version->allocation.host.resource->Map(0, &range, reinterpret_cast<void**>(&version->dwords));
    
    ASSERT(dwordOffset <= version->dwordCount, "Unexpected write buffer");
    device->virtualAddressTable.Unlock();
}

void VirtualAddressMappingTable::MapHandle(VirtualAddressMappingTablePersistentVersion* version, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    // Full structured range
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};
    view.Format = DXGI_FORMAT_R32_UINT;
    view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Buffer.NumElements = version->dwordCount;
    view.Buffer.FirstElement = 0;
    
    // Create view
    device->object->CreateShaderResourceView(
        version->allocation.device.resource,
        &view,
        handle
    );
}

void VirtualAddressMappingTable::Destruct(VirtualAddressMappingTablePersistentVersion *version) {
    if (version->allocation.device.resource) {
        deviceAllocator->Free(version->allocation);
    }
    
    delete version;
}

void VirtualAddressMappingTable::Prune(uint64_t count) {
    freeVersions.erase(
        std::remove_if(
            freeVersions.begin(), freeVersions.end(),
            [&](VirtualAddressMappingTablePersistentVersion* version) {
                if (version->entryCount < count) {
                    Destruct(version);
                    return true;
                }
                return false;
            }
        ),
        freeVersions.end()
    );
}
