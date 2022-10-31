#pragma once

// Backend
#include "VirtualResourceMapping.h"
#include <Backends/DX12/Allocation/MirrorAllocation.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

class DeviceAllocator;

/// Performs mapping between virtual heaps and physical resources
class PhysicalResourceMappingTable {
public:
    PhysicalResourceMappingTable(const ComRef<DeviceAllocator>& allocator);

    /// Install the table
    /// \param count number of descriptors
    void Install(uint32_t count);

    /// Update the table for use on a given list
    /// \param list list to be updated on
    void Update(ID3D12GraphicsCommandList* list);

    /// Modify mappings at a given range
    /// \param offset starting index
    /// \param count number of mappings
    /// \return mapping base
    VirtualResourceMapping* ModifyMappings(uint32_t offset, uint32_t count);

    /// Get the mappings at a given range
    /// \param offset starting index
    /// \param count number of mappings
    /// \return mapping base
    const VirtualResourceMapping* GetMappings(uint32_t offset, uint32_t count);

    /// Write a single mapping at a given offset
    /// \param offset offset to be written
    /// \param mapping mapping to write
    void WriteMapping(uint32_t offset, const VirtualResourceMapping& mapping);

    /// Get the underlying resource
    /// \return
    ID3D12Resource* GetResource() const {
        return allocation.device.resource;
    }

    /// Get the descriptor view
    /// \return
    const D3D12_SHADER_RESOURCE_VIEW_DESC& GetView() const {
        return view;
    }

private:
    /// Does this table need updating?
    bool isDirty{true};

    /// Number of mappings contained
    uint32_t virtualMappingCount{0};

    /// Mapped virtual entries
    VirtualResourceMapping* virtualMappings{nullptr};

    /// Underlying allocation
    MirrorAllocation allocation;

    /// Allocation view
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};

private:
    ComRef<DeviceAllocator> allocator{};
};
