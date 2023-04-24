#pragma once

// Backend
#include "VirtualResourceMapping.h"
#include "PhysicalResourceMappingTableSegment.h"
#include "PhysicalResourceSegment.h"
#include <Backends/Vulkan/Allocation/MirrorAllocation.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <vector>

// Forward declarations
struct DeviceDispatchTable;
class DeviceAllocator;

/// Performs mapping between virtual descriptors and physical resources
class PhysicalResourceMappingTable : public TComponent<PhysicalResourceMappingTable> {
public:
    COMPONENT(PhysicalResourceMappingTable);

    PhysicalResourceMappingTable(DeviceDispatchTable* table);
    ~PhysicalResourceMappingTable();

    /// Install the table
    bool Install();

    /// Update the table for use on a given list
    /// \param commandBuffer buffer to be updated on
    void Update(VkCommandBuffer commandBuffer);

    /// Allocate a new segment
    /// \param count number of descriptors
    /// \return segment identifier
    PhysicalResourceSegmentID Allocate(uint32_t count);

    /// Free a segment
    /// \param id identifier to be freed
    void Free(PhysicalResourceSegmentID id);

    /// Get the shader offset for a given segment
    /// \param id segment identifier
    /// \return shader buffer offset
    PhysicalResourceMappingTableSegment GetSegmentShader(PhysicalResourceSegmentID id);

    /// Write a single mapping at a given offset
    /// \param offset offset to be written
    /// \param mapping mapping to write
    VirtualResourceMapping* ModifyMappings(PhysicalResourceSegmentID id, uint32_t offset, uint32_t count);

    /// Write a single mapping at a given offset
    /// \param offset offset to be written
    /// \param mapping mapping to write
    void WriteMapping(PhysicalResourceSegmentID id, uint32_t offset, const VirtualResourceMapping& mapping);

    /// Get an existing mapping within a segment
    /// \param id segment identifier
    /// \param offset offset within the segment
    /// \return written mapping
    VirtualResourceMapping GetMapping(PhysicalResourceSegmentID id, uint32_t offset) const;

    /// Get the underlying buffer
    VkBuffer GetDeviceBuffer() const {
        return deviceBuffer;
    }

    /// Get the descriptor view
    const VkBufferView GetDeviceView() const {
        return deviceView;
    }

private:
    /// Get the current offset
    /// \return element offset
    uint32_t GetHeadOffset() const;

    /// Allocate a new table with a given size
    /// \param count number of descriptors
    void AllocateTable(uint32_t count);

    /// Defragment the table
    void Defragment();

private:
    /// Does this table need updating?
    bool isDirty{true};

    /// Number of mappings contained
    uint32_t virtualMappingCount{0};

    /// Mapped virtual entries
    VirtualResourceMapping* virtualMappings{nullptr};

    /// Underlying allocation
    MirrorAllocation allocation;

    /// Buffer handles
    VkBuffer hostBuffer{nullptr};
    VkBuffer deviceBuffer{nullptr};

    /// Descriptor handles
    VkBufferView deviceView{nullptr};

private:
    /// Free indices to be used immediately
    std::vector<PhysicalResourceSegmentID> freeIndices;

    /// All indices, sparsely populated
    std::vector<uint32_t> indices;

    /// Linear segments
    std::vector<PhysicalResourceMappingTableSegment> segments;

    /// Number of live segments
    uint32_t liveSegmentCount{0};

    /// Current fragmentation
    uint32_t fragmentedEntries{0};

private:
    DeviceDispatchTable* table;

    /// Components
    ComRef<DeviceAllocator> deviceAllocator{};
};
