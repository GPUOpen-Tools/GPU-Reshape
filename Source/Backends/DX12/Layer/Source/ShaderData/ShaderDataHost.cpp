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

#include <Backends/DX12/ShaderData/ShaderDataHost.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Translation.h>
#include <Backends/DX12/Resource/ReservedConstantData.h>
#include <Backend/ShaderData/ShaderDataValidationCoverage.h>

ShaderDataHost::ShaderDataHost(DeviceState *device) :
    device(device),
    freeIndices(device->allocators.Tag(kAllocShaderData)),
    indices(device->allocators.Tag(kAllocShaderData)),
    resources(device->allocators.Tag(kAllocShaderData)),
    freeMappingIndices(device->allocators.Tag(kAllocShaderData)),
    mappings(device->allocators.Tag(kAllocShaderData)) {

}

ShaderDataHost::~ShaderDataHost() {
    // Release resources
    for (const ResourceEntry& entry : resources) {
        if (entry.allocation.resource) {
            if (entry.allocation.allocation) {
                entry.allocation.allocation->Release();
            }
            entry.allocation.resource->Release();
        }
    }
}

bool ShaderDataHost::Install() {
    // Query device options
    if (FAILED(device->object->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)))) {
        return false;
    }

    // Query virtual address support
    if (FAILED(device->object->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &virtualAddressOptions, sizeof(virtualAddressOptions)))) {
        virtualAddressOptions.MaxGPUVirtualAddressBitsPerProcess  = UINT32_MAX;
        virtualAddressOptions.MaxGPUVirtualAddressBitsPerResource = UINT32_MAX;
    }

    // Fill capability table
    capabilityTable.supportsTiledResources = (options.TiledResourcesTier != D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED);
    capabilityTable.bufferMaxElementCount = UINT64_MAX;

    // TODO: This is not correct
    capabilityTable.bufferMaxSize = UINT64_MAX;

    // Install coverage support
    ComRef coverage = registry->AddNew<ShaderDataValidationCoverage>();
    coverage->Install();

    // OK
    return true;
}

ShaderDataID ShaderDataHost::CreateBuffer(const ShaderDataBufferInfo &info, const char* name) {
    std::lock_guard guard(mutex);
    
    // Determine index
    ShaderDataID rid;
    if (freeIndices.empty()) {
        // Allocate at end
        rid = static_cast<uint32_t>(indices.size());
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Mapped description
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Alignment = 0;
    desc.Width = Backend::IL::GetSize(info.format) * info.elementCount;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.MipLevels = 1;
    desc.SampleDesc.Quality = 0;
    desc.SampleDesc.Count = 1;

    // Always UAV-capable, unless host
    if (!(info.flagSet & ShaderDataBufferFlag::Host)) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    // Set index
    indices[rid] = static_cast<uint32_t>(resources.size());

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.info.id = rid;
    entry.info.type = ShaderDataType::Buffer;
    entry.info.buffer = info;

    // If tiled, create the reserved resource immediately
    if (info.flagSet & ShaderDataBufferFlag::Tiled) {
        device->object->CreateReservedResource(
            &desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            __uuidof(ID3D12Resource*),
            reinterpret_cast<void**>(&entry.allocation.resource)
        );
    } else {
        AllocationResidency residency;

        // Translate residency
        if (info.flagSet & ShaderDataBufferFlag::Host) {
            residency = AllocationResidency::HostReadback;
            entry.isNonDescriptor = true;
        } else if (info.flagSet & ShaderDataBufferFlag::HostVisible) {
            residency = AllocationResidency::HostVisible;
            entry.isNonDescriptor = info.flagSet & ShaderDataBufferFlag::NonDescriptor;
        } else {
            residency = AllocationResidency::Device;
        }
        
        entry.allocation = device->deviceAllocator->Allocate(desc, residency);
    }

#ifndef NDEBUG
    wchar_t nameWide[256];
    if (size_t length = std::strlen(name); !mbstowcs_s(&length, nameWide, name, length)) {
        entry.allocation.resource->SetName(nameWide);
    }
#endif // NDEBUG

    // OK
    return rid;
}

ShaderDataID ShaderDataHost::CreateBufferBinding(const ShaderProgramID& programID, const ShaderDataBufferBindingInfo &info) {
    std::lock_guard guard(mutex);

    // Determine index
    ShaderDataID rid;
    if (freeIndices.empty()) {
        // Allocate at end
        rid = static_cast<uint32_t>(indices.size());
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Set index
    indices[rid] = static_cast<uint32_t>(resources.size());

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.info.id = rid;
    entry.info.type = ShaderDataType::BufferBinding;
    entry.info.bufferBinding = info;

    // Set per program binding
    ProgramEntry& program = programs[programID];
    program.shaderDataIDs.push_back(rid);

    // OK
    return rid;
}

ShaderDataID ShaderDataHost::CreateEventData(const ShaderDataEventInfo &info) {
    std::lock_guard guard(mutex);
    
    // Determine index
    ShaderDataID rid;
    if (freeIndices.empty()) {
        // Allocate at end
        rid = static_cast<uint32_t>(indices.size());
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Set index
    indices[rid] = static_cast<uint32_t>(resources.size());

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.allocation = {};
    entry.info.id = rid;
    entry.info.type = ShaderDataType::Event;
    entry.info.event = info;

    // OK
    return rid;
}

ShaderDataID ShaderDataHost::CreateDescriptorData(const ShaderDataDescriptorInfo &info) {
    std::lock_guard guard(mutex);
    
    // Determine index
    ShaderDataID rid;
    if (freeIndices.empty()) {
        // Allocate at end
        rid = static_cast<uint32_t>(indices.size());
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Set index
    indices[rid] = static_cast<uint32_t>(resources.size());

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.allocation = {};
    entry.info.id = rid;
    entry.info.type = ShaderDataType::Descriptor;
    entry.info.descriptor = info;

    // OK
    return rid;
}

void *ShaderDataHost::Map(ShaderDataID rid) {
    std::lock_guard guard(mutex);
    uint32_t index = indices[rid];

    // Entry to map
    ResourceEntry &entry = resources[index];

    // Map it!
    return device->deviceAllocator->Map(entry.allocation);
}

void ShaderDataHost::Unmap(ShaderDataID rid, void *mapped) {
    std::lock_guard guard(mutex);
    uint32_t index = indices[rid];

    // Entry to map
    ResourceEntry &entry = resources[index];
    device->deviceAllocator->Unmap(entry.allocation);
}

ShaderDataMappingID ShaderDataHost::CreateMapping(ShaderDataID data, uint64_t tileCount) {
    std::lock_guard guard(mutex);
    
    // Allocate index
    ShaderDataMappingID mid;
    if (freeMappingIndices.empty()) {
        mid = static_cast<uint32_t>(mappings.size());
        mappings.emplace_back();
    } else {
        mid = freeMappingIndices.back();
        freeMappingIndices.pop_back();
    }

    // Create allocation
    MappingEntry& entry = mappings[mid];
    entry.allocation = device->deviceAllocator->AllocateMemory(kShaderDataMappingTileWidth, kShaderDataMappingTileWidth * tileCount);

    // OK
    return mid;
}

void ShaderDataHost::DestroyMapping(ShaderDataMappingID mid) {
    std::lock_guard guard(mutex);
    
    MappingEntry& entry = mappings[mid];

    // Release the allocation
    device->deviceAllocator->Free(entry.allocation);
    entry.allocation = nullptr;

    // Mark as free
    freeMappingIndices.push_back(mid);
}

void ShaderDataHost::FlushMappedRange(ShaderDataID rid, size_t offset, size_t length) {
    std::lock_guard guard(mutex);
    uint32_t index = indices[rid];

    // Entry to flush
    ResourceEntry &entry = resources[index];

    // Flush the range
    device->deviceAllocator->FlushMappedRange(entry.allocation, offset, length);
}

Allocation ShaderDataHost::GetResourceAllocation(ShaderDataID rid) {
    std::lock_guard guard(mutex);
    uint32_t index = indices[rid];

    // Entry to map
    ResourceEntry &entry = resources[index];

    // OK
    return entry.allocation;
}

D3D12MA::Allocation * ShaderDataHost::GetMappingAllocation(ShaderDataMappingID mid) {
    std::lock_guard guard(mutex);
    MappingEntry &entry = mappings[mid];
    return entry.allocation;
}

uint32_t ShaderDataHost::GetBindingIndex(ShaderProgramID programID, ShaderDataID rid) {
    std::lock_guard guard(mutex);

    // Get program
    ASSERT(programs.contains(programID), "Program not registered");
    ProgramEntry& program = programs[programID];

    // Find the index of the RID
    for (uint32_t i = 0; i < program.shaderDataIDs.size(); i++) {
        if (program.shaderDataIDs[i] == rid) {
            return i;
        }
    }

    ASSERT(false, "Shader data not registered");
    return InvalidShaderDataID;
}

void ShaderDataHost::Destroy(ShaderDataID rid) {
    std::lock_guard guard(mutex);
    uint32_t index = indices[rid];

    // Entry to release
    ResourceEntry &entry = resources[index];

    // Release optional resource
    if (entry.allocation.resource) {
        entry.allocation.allocation->Release();
        entry.allocation.resource->Release();
    }

    // Not last element?
    if (index != resources.size() - 1) {
        const ResourceEntry &back = resources.back();

        // Swap move last element to current position
        indices[back.info.id] = index;

        // Update indices
        resources[index] = back;
        resources.pop_back();
    }

    resources.pop_back();

    // Add as free index
    freeIndices.push_back(rid);
}

void ShaderDataHost::EnumerateShader(uint32_t *count, ShaderDataInfo *out, ShaderDataTypeSet mask) {
    std::lock_guard guard(mutex);
    
    if (out) {
        uint32_t offset = 0;

        for (uint32_t i = 0; i < resources.size(); i++) {
            if (resources[i].isNonDescriptor) {
                continue;
            }
            
            if (mask & resources[i].info.type) {
                out[offset++] = resources[i].info;
            }
        }
    } else {
        uint32_t value = 0;

        for (uint32_t i = 0; i < resources.size(); i++) {
            if (resources[i].isNonDescriptor) {
                continue;
            }
            
            if (mask & resources[i].info.type) {
                value++;
            }
        }

        *count = value;
    }
}

void ShaderDataHost::EnumerateProgram(ShaderProgramID programID, uint32_t *count, ShaderDataInfo *out, ShaderDataTypeSet mask) {
    std::lock_guard guard(mutex);

    // Find program
    auto it = programs.find(programID);
    if (it == programs.end()) {
        *count = 0;
        return;
    }
    
    if (out) {
        uint32_t offset = 0;

        for (uint32_t i = 0; i < it->second.shaderDataIDs.size(); i++) {
            const ResourceEntry& resource = resources[indices[it->second.shaderDataIDs[i]]];

            // Is of type?
            if (mask & resource.info.type) {
                out[offset++] = resource.info;
            }
        }
    } else {
        uint32_t value = 0;

        for (uint32_t i = 0; i < it->second.shaderDataIDs.size(); i++) {
            const ResourceEntry& resource = resources[indices[it->second.shaderDataIDs[i]]];
            
            // Is of type?
            if (mask & resource.info.type) {
                value++;
            }
        }

        *count = value;
    }
}

void ShaderDataHost::CreateDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptorHandle, uint32_t stride) {
    std::lock_guard guard(mutex);
    
    uint32_t offset = 0;

    // Max number of addressable bytes (-3 for bits to bytes)
    size_t maxVirtualAddressBytes = 1ull << (options.MaxGPUVirtualAddressBitsPerResource - 3u);

    for (uint32_t i = 0; i < resources.size(); i++) {
        const ResourceEntry &entry = resources[i];

        if (!(ShaderDataType::DescriptorMask & entry.info.type)) {
            continue;
        }
        
        if (resources[i].isNonDescriptor) {
            continue;
        }

        switch (entry.info.type) {
            default: {
                ASSERT(false, "Invalid resource");
                break;
            }
            case ShaderDataType::Buffer: {
                // Setup view
                D3D12_UNORDERED_ACCESS_VIEW_DESC view{};
                view.Format = Translate(entry.info.buffer.format);
                view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                view.Buffer.FirstElement = 0;
                view.Buffer.StructureByteStride = 0;

                // Limit number of elements by the actual number of addressable elements
                size_t maxElements = maxVirtualAddressBytes / Backend::IL::GetSize(entry.info.buffer.format);
                view.Buffer.NumElements = static_cast<uint32_t>(std::min(entry.info.buffer.elementCount, maxElements));

                // Workaround for runtime bug that assumes 32 bit indexing, this has since been fixed in later agility SDKs.
                // Platform specific workaround for large UAVs (vaddr > 32 bits), seems like a bug?
                if (!device->sdk.isAgilitySDKOverride714 || device->vendor == Backend::VendorType::Nvidia) {
                    view.Buffer.NumElements = std::min(view.Buffer.NumElements, UINT32_MAX / Backend::IL::GetSize(entry.info.buffer.format) - static_cast<uint32_t>(sizeof(uint64_t)));
                }
                
                // Create descriptor
                device->object->CreateUnorderedAccessView(
                    entry.allocation.resource, nullptr,
                    &view,
                    D3D12_CPU_DESCRIPTOR_HANDLE{.ptr = baseDescriptorHandle.ptr + stride * offset}
                );
                break;
            }
            case ShaderDataType::Texture: {
                ASSERT(false, "Not implemented");
                break;
            }
        }

        // Next!
        offset++;
    }
}

ShaderDataCapabilityTable ShaderDataHost::GetCapabilityTable() {
    return capabilityTable;
}

ConstantShaderDataBuffer ShaderDataHost::CreateConstantDataBuffer() {
    std::lock_guard guard(mutex);
    
    ConstantShaderDataBuffer out;

    // Total dword count
    uint32_t dwordCount = 0;

    // Reserved data
    dwordCount += static_cast<uint32_t>(ReservedConstantDataDWords::Prefix);

    // Summarize size
    for (uint32_t i = 0; i < resources.size(); i++) {
        if (resources[i].info.type == ShaderDataType::Descriptor) {
            dwordCount += resources[i].info.descriptor.dwordCount;
        }
    }

    // Disallow dummy buffer
    if (!dwordCount) {
        return {};
    }

    // Minimum length of constant data
    size_t length = sizeof(uint32_t) * dwordCount;

    // Align length
    const size_t alignSub1 = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1;
    length = (length + alignSub1) & ~alignSub1;

    // Mapped description
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = length;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.MipLevels = 1;
    desc.SampleDesc.Quality = 0;
    desc.SampleDesc.Count = 1;

    // Allocate buffer data on host, let the drivers handle page swapping
    out.allocation = device->deviceAllocator->Allocate(desc, AllocationResidency::HostVisible);

#ifndef NDEBUG
    out.allocation.resource->SetName(L"ShaderDataHostConstantDataBuffer");
#endif // NDEBUG

    // Set up view
    out.view.BufferLocation = out.allocation.resource->GetGPUVirtualAddress();
    out.view.SizeInBytes = static_cast<uint32_t>(desc.Width);

    // OK
    return out;
}

ShaderConstantsRemappingTable ShaderDataHost::CreateConstantMappingTable() {
    std::lock_guard guard(mutex);
    
    ShaderConstantsRemappingTable out(indices.size());

    // Current offset
    uint32_t dwordOffset = 0;

    // Offset by reserved data
    dwordOffset += static_cast<uint32_t>(ReservedConstantDataDWords::Prefix);

    // Accumulate offsets
    for (uint32_t i = 0; i < resources.size(); i++) {
        if (resources[i].info.type == ShaderDataType::Descriptor) {
            out[resources[i].info.id] = dwordOffset;
            dwordOffset += resources[i].info.descriptor.dwordCount;
        }
    }

    // OK
    return out;
}
