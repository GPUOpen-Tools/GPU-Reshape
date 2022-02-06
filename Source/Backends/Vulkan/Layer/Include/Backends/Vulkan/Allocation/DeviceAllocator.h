#pragma once

// Layer
#include <Backends/Vulkan/VMA.h>
#include <Backends/Vulkan/Allocation/Allocation.h>
#include <Backends/Vulkan/Allocation/MirrorAllocation.h>
#include <Backends/Vulkan/Allocation/Residency.h>

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
    void Install(DeviceDispatchTable* table);

    /// Create a new allocation
    /// \param size byte size of the allocation
    /// \param residency expected residency of the allocation
    /// \return the allocation
    Allocation Allocate(const VkMemoryRequirements& requirements, AllocationResidency residency = AllocationResidency::Device);

    /// Create a new mirror allocation
    /// \param size byte size of the allocation
    /// \param residency expected residency of the allocation, if Host, the allocations are shared
    /// \return the mirror allocation
    MirrorAllocation AllocateMirror(const VkMemoryRequirements& requirements, AllocationResidency residency = AllocationResidency::Device);

    /// Free a given allocation
    /// \param allocation
    void Free(const Allocation& allocation);

    /// Free a given mirror allocation
    /// \param mirrorAllocation
    void Free(const MirrorAllocation& mirrorAllocation);

    /// Bind a buffer allocation
    /// \param allocation the allocation to be bound
    /// \param buffer the buffer to bind with
    /// \return success state
    bool BindBuffer(const Allocation& allocation, VkBuffer buffer);

    /// Map an allocation
    /// \param allocation the allocation to be mapped
    /// \return mapped address, nullptr if failed
    void* Map(const Allocation& allocation);

    /// Unmap a mapped allocation
    /// \param allocation the mapped allocation
    void Unmap(const Allocation& allocation);

private:
    VmaAllocator allocator{nullptr};
};