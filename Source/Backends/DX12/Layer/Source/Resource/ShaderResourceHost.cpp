#include <Backends/DX12/Resource/ShaderResourceHost.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Translation.h>

ShaderResourceHost::ShaderResourceHost(DeviceState *device) : device(device) {

}

bool ShaderResourceHost::Install() {
    return true;
}

ShaderResourceID ShaderResourceHost::CreateBuffer(const ShaderBufferInfo &info) {
    // Determine index
    ShaderResourceID rid;
    if (freeIndices.empty()) {
        // Allocate at end
        rid = indices.size();
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Mapped description
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    desc.Alignment = 0;
    desc.Width = Backend::IL::GetSize(info.format) * info.elementCount;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.MipLevels = 1;
    desc.SampleDesc.Quality = 0;
    desc.SampleDesc.Count = 1;

    // Set index
    indices[rid] = resources.size();

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.allocation = device->deviceAllocator->Allocate(desc);
    entry.info.id = rid;
    entry.info.type = ShaderResourceType::Buffer;
    entry.info.buffer = info;

    // OK
    return rid;
}

void ShaderResourceHost::DestroyBuffer(ShaderResourceID rid) {
    uint32_t index = indices[rid];

    // Release entry
    ResourceEntry &entry = resources[index];
    entry.allocation.allocation->Release();
    entry.allocation.resource->Release();

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

void ShaderResourceHost::Enumerate(uint32_t *count, ShaderResourceInfo *out) {
    if (out) {
        for (uint32_t i = 0; i < *count; i++) {
            out[i] = resources[i].info;
            i++;
        }
    } else {
        *count = resources.size();
    }
}

void ShaderResourceHost::CreateDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptorHandle, uint32_t stride) {
    for (uint32_t i = 0; i < resources.size(); i++) {
        const ResourceEntry &entry = resources[i];

        switch (entry.info.type) {
            default: {
                ASSERT(false, "Invalid resource");
                break;
            }
            case ShaderResourceType::Buffer: {
                // Setup view
                D3D12_UNORDERED_ACCESS_VIEW_DESC view{};
                view.Format = Translate(entry.info.buffer.format);
                view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                view.Buffer.FirstElement = 0;
                view.Buffer.NumElements = entry.info.buffer.elementCount;
                view.Buffer.StructureByteStride = 0;

                // Create descriptor
                device->object->CreateUnorderedAccessView(
                    entry.allocation.resource, nullptr,
                    &view,
                    D3D12_CPU_DESCRIPTOR_HANDLE{.ptr = baseDescriptorHandle.ptr + stride * i}
                );
                break;
            }
            case ShaderResourceType::Texture: {
                ASSERT(false, "Not implemented");
                break;
            }
        }
    }
}
