// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#pragma once

// Backend
#include "VirtualResourceMapping.h"
#include "PhysicalResourceMappingTableSegment.h"
#include "PhysicalResourceSegment.h"
#include "PhysicalResourceMappingTableQueueState.h"
#include <Backends/Vulkan/Allocation/MirrorAllocation.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>

// Common
#include <Common/Containers/PartitionedAllocator.h>
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
class PhysicalResourceMappingTablePersistentVersion;
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
    /// \param queueState queue state
    /// \return persistent version, must be manually released
    PhysicalResourceMappingTablePersistentVersion* GetPersistentVersion(VkCommandBuffer commandBuffer, PhysicalResourceMappingTableQueueState* queueState);

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
    void WriteMapping(PhysicalResourceSegmentID id, uint32_t offset, const VirtualResourceMapping& mapping);

    /// Get an existing mapping within a segment
    /// \param id segment identifier
    /// \param offset offset within the segment
    /// \return written mapping
    VirtualResourceMapping GetMapping(PhysicalResourceSegmentID id, uint32_t offset);

    /// Get the mapping offset within a segment
    /// \param id segment identifier
    /// \param offset offset within the segment
    /// \return mapping offset
    size_t GetMappingOffset(PhysicalResourceSegmentID id, uint32_t offset);

    /// \brief Copy all mappings, must be of same length
    /// \param source source segment
    /// \param dest destination segment
    void CopyMappings(PhysicalResourceSegmentID source, PhysicalResourceSegmentID dest);

private:
    /// Allocate a new table with a given size
    /// \param count number of descriptors
    void AllocateTable(uint32_t count);

private:
    /// Table (global) commit head
    size_t commitHead{0};

    /// Number of mappings contained
    uint32_t virtualMappingCount{0};

    /// Current persistent version
    PhysicalResourceMappingTablePersistentVersion* persistentVersion{nullptr};

    /// Partitioned allocator for reduced fragmentation
    PartitionedAllocator<
        12,   /// Total of 12 partition levels
        4096, /// Each partition to 4096 elements
        512   /// Slack of 512 on large blocks
    > partitionedAllocator;

private:
    /// Free indices to be used immediately
    std::vector<PhysicalResourceSegmentID> freeIndices;

    /// All indices, sparsely populated
    std::vector<uint32_t> indices;

    /// Linear segments
    std::vector<PhysicalResourceMappingTableSegment> segments;

    /// Number of live segments
    uint32_t liveSegmentCount{0};

private:
    DeviceDispatchTable* table;

    /// Shared lock
    std::mutex mutex;

    /// Components
    ComRef<DeviceAllocator> deviceAllocator{};
};
