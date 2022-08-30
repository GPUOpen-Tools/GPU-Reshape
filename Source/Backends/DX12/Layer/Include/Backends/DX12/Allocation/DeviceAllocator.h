#pragma once

// Layer
#include <Backends/DX12/Allocation/Allocation.h>
#include <Backends/DX12/Allocation/MirrorAllocation.h>
#include <Backends/DX12/Allocation/Residency.h>

// D3D12MA
#include <D3D12MemAlloc.h>

// Common
#include <Common/IComponent.h>

// Forward declarations
struct DeviceDispatchTable;

/// Handles device memory allocations and binding
class DeviceAllocator : public TComponent<DeviceAllocator> {
public:
    COMPONENT(DeviceAllocator);

    ~DeviceAllocator();

    /// Install this allocator
    /// \param table
    bool Install(ID3D12Device* device, IDXGIAdapter* adapter);

    /// Create a new allocation
    /// \param size byte size of the allocation
    /// \param residency expected residency of the allocation
    /// \return the allocation
    Allocation Allocate(const D3D12_RESOURCE_DESC& desc, AllocationResidency residency = AllocationResidency::Device);

    /// Create a new mirror allocation
    /// \param size byte size of the allocation
    /// \param residency expected residency of the allocation, if Host, the allocations are shared
    /// \return the mirror allocation
    MirrorAllocation AllocateMirror(const D3D12_RESOURCE_DESC& desc, AllocationResidency residency = AllocationResidency::Device);

    /// Free a given allocation
    /// \param allocation
    void Free(const Allocation& allocation);

    /// Free a given mirror allocation
    /// \param mirrorAllocation
    void Free(const MirrorAllocation& mirrorAllocation);

    /// Map an allocation
    /// \param allocation the allocation to be mapped
    /// \return mapped address, nullptr if failed
    void* Map(const Allocation& allocation);

    /// Unmap a mapped allocation
    /// \param allocation the mapped allocation
    void Unmap(const Allocation& allocation);

private:
    /// Underlying allocator
    D3D12MA::Allocator* allocator{nullptr};

    /// Special host pool
    D3D12MA::Pool* wcHostPool{nullptr};
};