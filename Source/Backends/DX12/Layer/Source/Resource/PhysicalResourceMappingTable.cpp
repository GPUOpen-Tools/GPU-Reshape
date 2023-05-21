#include <Backends/DX12/Resource/PhysicalResourceMappingTable.h>
#include <Backends/DX12/Allocation/DeviceAllocator.h>

PhysicalResourceMappingTable::PhysicalResourceMappingTable(const Allocators& allocators, const ComRef<DeviceAllocator> &allocator) : states(allocators), allocator(allocator) {

}

void PhysicalResourceMappingTable::Install(uint32_t count) {
    std::lock_guard guard(mutex);
    virtualMappingCount = count;

    // Mapped description
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = sizeof(uint32_t) * count;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.MipLevels = 1;
    desc.SampleDesc.Quality = 0;
    desc.SampleDesc.Count = 1;

    // Create allocation
    allocation = allocator->AllocateMirror(desc);

    // Setup view
    view.Format = DXGI_FORMAT_R32_UINT;
    view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    view.Buffer.FirstElement = 0;
    view.Buffer.NumElements = count;
    view.Buffer.StructureByteStride = 0;

    // Opaque host data
    void* mappedOpaque{nullptr};

    // Map range
    D3D12_RANGE range;
    range.Begin = 0;
    range.End = sizeof(uint32_t) * count;
    allocation.host.resource->Map(0, &range, &mappedOpaque);

    // Store host
    virtualMappings = static_cast<VirtualResourceMapping*>(mappedOpaque);

    // Dummy initialize all VRMs
    for (uint32_t i = 0; i < count; i++) {
        virtualMappings[i] = VirtualResourceMapping{
            .puid = IL::kResourceTokenPUIDInvalidUndefined,
            .type = 0,
            .srb = 0
        };
    }

    // Zero states
    states.resize(count, nullptr);
}

void PhysicalResourceMappingTable::Update(ID3D12GraphicsCommandList *list) {
    std::lock_guard guard(mutex);

    // May not need updates
    if (!isDirty) {
        return;
    }

    // Flush all pending work and transition to src
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = allocation.host.resource;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    list->ResourceBarrier(1u, &barrier);

    // Copy host data to device
    list->CopyBufferRegion(allocation.device.resource, 0u, allocation.host.resource, 0, sizeof(uint32_t) * virtualMappingCount);

    // Flush all pending work and transition to src
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    list->ResourceBarrier(1u, &barrier);

    // OK
    isDirty = false;
}

void PhysicalResourceMappingTable::WriteMapping(uint32_t offset, const VirtualResourceMapping &mapping) {
    std::lock_guard guard(mutex);

    ASSERT(offset < virtualMappingCount, "Out of bounds mapping");
    virtualMappings[offset] = mapping;
    
    isDirty = true;
}

void PhysicalResourceMappingTable::SetMappingState(uint32_t offset, ResourceState *state) {
    std::lock_guard guard(mutex);
    ASSERT(offset < virtualMappingCount, "Out of bounds mapping");
    states[offset] = state;
}

ResourceState *PhysicalResourceMappingTable::GetMappingState(uint32_t offset) {
    std::lock_guard guard(mutex);
    ASSERT(offset < virtualMappingCount, "Out of bounds mapping");
    return states[offset];
}

void PhysicalResourceMappingTable::WriteMapping(uint32_t offset, ResourceState *state, const VirtualResourceMapping &mapping) {
    std::lock_guard guard(mutex);

    ASSERT(offset < virtualMappingCount, "Out of bounds mapping");
    
    virtualMappings[offset] = mapping;
    states[offset] = state;
    
    isDirty = true;
}
