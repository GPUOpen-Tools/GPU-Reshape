#include <Backends/DX12/ShaderData/ShaderDataHost.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Translation.h>

ShaderDataHost::ShaderDataHost(DeviceState *device) : device(device) {

}

bool ShaderDataHost::Install() {
    return true;
}

ShaderDataID ShaderDataHost::CreateBuffer(const ShaderDataBufferInfo &info) {
    // Determine index
    ShaderDataID rid;
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
    entry.info.type = ShaderDataType::Buffer;
    entry.info.buffer = info;

    // OK
    return rid;
}

ShaderDataID ShaderDataHost::CreateEventData(const ShaderDataEventInfo &info) {
    // Determine index
    ShaderDataID rid;
    if (freeIndices.empty()) {
        // Allocate at end
        rid = indices.size();
        indices.emplace_back();
    } else {
        // Consume free index
        rid = freeIndices.back();
        freeIndices.pop_back();
    }

    // Set index
    indices[rid] = resources.size();

    // Create allocation
    ResourceEntry &entry = resources.emplace_back();
    entry.allocation = {};
    entry.info.id = rid;
    entry.info.type = ShaderDataType::Event;
    entry.info.event = info;

    // OK
    return rid;
}

void ShaderDataHost::Destroy(ShaderDataID rid) {
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

void ShaderDataHost::Enumerate(uint32_t *count, ShaderDataInfo *out, ShaderDataTypeSet mask) {
    if (out) {
        uint32_t offset = 0;

        for (uint32_t i = 0; i < resources.size(); i++) {
            if (mask & resources[i].info.type) {
                out[offset++] = resources[i].info;
            }
        }
    } else {
        uint32_t value = 0;

        for (uint32_t i = 0; i < resources.size(); i++) {
            if (mask & resources[i].info.type) {
                value++;
            }
        }

        *count = value;
    }
}

void ShaderDataHost::CreateDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptorHandle, uint32_t stride) {
    uint32_t offset = 0;

    for (uint32_t i = 0; i < resources.size(); i++) {
        const ResourceEntry &entry = resources[i];

        if (!(ShaderDataType::DescriptorMask & entry.info.type)) {
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
                view.Buffer.NumElements = entry.info.buffer.elementCount;
                view.Buffer.StructureByteStride = 0;

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
