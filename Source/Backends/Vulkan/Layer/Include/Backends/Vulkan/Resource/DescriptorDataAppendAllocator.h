#pragma once

// Layer
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include "DescriptorDataSegment.h"

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

class DescriptorDataAppendAllocator {
public:
    DescriptorDataAppendAllocator(DeviceDispatchTable* table, const ComRef<DeviceAllocator>& allocator, size_t maxChunkSize) : table(table), allocator(allocator), maxChunkSize(maxChunkSize) {

    }

    /// Set the chunk
    /// \param segmentEntry segment to be bound to
    void SetChunk(const DescriptorDataSegmentEntry& segmentEntry) {
        // Set inherited width
        chunkSize = segmentEntry.width / sizeof(uint32_t);

        // Add entry
        segment.entries.emplace_back(segmentEntry);

        // Set mapped
        mapped = static_cast<uint32_t*>(allocator->Map(segmentEntry.allocation.host));
    }

    /// Begin a new segment
    /// \param rootCount number of root parameters
    void BeginSegment(uint32_t rootCount) {
        pendingRootCount = rootCount;
        pendingRoll = true;
    }

    /// Set a root value
    /// \param offset current root offset
    /// \param value value at root offset
    void Set(uint32_t offset, uint32_t value) {
        if (pendingRoll) {
            RollChunk();
        }

        ASSERT(offset < mappedSegmentLength, "Out of bounds descriptor segment offset");
        mapped[mappedOffset + offset] = value;
    }

    /// Set a root value
    /// \param offset current root offset
    /// \param value value at root offset
    void SetOrAllocate(uint32_t offset, uint32_t allocationSize, uint32_t value) {
        // Begin a new segment if the previous does not suffice, may be allocated dynamically
        if (offset <= mappedSegmentLength) {
            ASSERT(allocationSize > offset, "Chunk allocation size must be larger than the expected offset");
            BeginSegment(allocationSize);
        }

        // Roll! D2! (Never played DnD, sorry)
        if (pendingRoll) {
            RollChunk();
        }

        ASSERT(offset < mappedSegmentLength, "Chunk allocation failed");
        mapped[mappedOffset + offset] = value;
    }

    /// Has this allocator been rolled? i.e. a new segment has begun
    /// \return
    bool HasRolled() const {
        return !pendingRoll;
    }

    /// Commit all changes for the GPU
    void Commit(CommandBufferObject* commandBuffer) {
        if (!mapped) {
            return;
        }

        // Get entry
        DescriptorDataSegmentEntry& entry = segment.entries.back();

        // Unmap range
        allocator->Unmap(entry.allocation.host);

        // Copy data to device if needed
        if (commandBuffer && entry.width) {
            VkBufferCopy copy;
            copy.srcOffset = 0;
            copy.dstOffset = 0;
            copy.size = entry.width;
            commandBuffer->dispatchTable.next_vkCmdCopyBuffer(commandBuffer->object, entry.bufferHost, entry.bufferDevice, 1u, &copy);
        }
    }

    /// Get the current segment address
    /// \return
    VkBuffer GetSegmentBuffer() const {
        return segment.entries.back().bufferDevice;
    }

    /// Get the current segment address
    /// \return
    uint64_t GetSegmentDynamicOffset() const {
        return mappedOffset;
    }

    /// Release the segment
    /// \return internal segment, ownership acquired
    DescriptorDataSegment ReleaseSegment() {
        // Reset internal state
        mappedOffset = 0;
        mappedSegmentLength = 0;
        chunkSize = 0;
        mapped = nullptr;

        // Release the segment
        return std::move(segment);
    }

private:
    /// Roll the current chunk
    void RollChunk() {
        // Advance current offset
        mappedOffset += mappedSegmentLength;

        // Out of memory?
        if (mappedOffset + pendingRootCount >= chunkSize) {
            // Growth factor of 1.5
            chunkSize = std::max<size_t>(64'000, static_cast<size_t>(chunkSize * 1.5f));

            // Account for device limits
            chunkSize = std::min<size_t>(chunkSize, maxChunkSize / sizeof(uint32_t));

            // Create new chunk
            CreateChunk();
        }

        // Set next roll length
        mappedSegmentLength = pendingRootCount;
        pendingRoll = false;
    }

    /// Create a new chunk
    void CreateChunk() {
        // Release existing chunk if needed
        if (mapped) {
            allocator->Unmap(segment.entries.back().allocation.host);
        }

        // Reset
        mappedOffset = 0;
        mappedSegmentLength = 0;

        // Next entry
        DescriptorDataSegmentEntry segmentEntry;
        segmentEntry.width = sizeof(uint32_t) * chunkSize;

        // Buffer info
        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.size = sizeof(uint32_t) * chunkSize;

        // Attempt to create the buffer
        if (table->next_vkCreateBuffer(table->object, &bufferInfo, nullptr, &segmentEntry.bufferDevice) != VK_SUCCESS) {
            return;
        }

        // Attempt to create the host buffer
        if (table->next_vkCreateBuffer(table->object, &bufferInfo, nullptr, &segmentEntry.bufferHost) != VK_SUCCESS) {
            return;
        }

        // Get the requirements
        VkMemoryRequirements requirements;
        table->next_vkGetBufferMemoryRequirements(table->object, segmentEntry.bufferDevice, &requirements);

        // Create the allocation
        segmentEntry.allocation = allocator->AllocateMirror(requirements);

        // Bind against the allocations
        allocator->BindBuffer(segmentEntry.allocation.device, segmentEntry.bufferDevice);
        allocator->BindBuffer(segmentEntry.allocation.host, segmentEntry.bufferHost);

        // Set as current chunk
        SetChunk(segmentEntry);
    }

private:
    /// Current mapping offset for the segment
    size_t mappedOffset{0};

    /// Current segment length
    size_t mappedSegmentLength{0};

    /// Total chunk size
    size_t chunkSize{0};

    /// Device chunk size limit
    size_t maxChunkSize{0};

    /// Root count requested for the next roll
    uint32_t pendingRootCount{0};

    /// Any pending roll?
    bool pendingRoll{true};

    /// Device allocator
    ComRef<DeviceAllocator> allocator;

    /// Parent table
    DeviceDispatchTable* table;

private:
    /// Current data segment, to be released later
    DescriptorDataSegment segment;

    /// Current mapped address of the segment
    uint32_t* mapped{nullptr};
};
