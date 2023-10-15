// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Layer
#include <Backends/Vulkan/Allocation/DeviceAllocator.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include <Backends/Vulkan/CommandBufferRenderPassScope.h>
#include "DescriptorDataSegment.h"

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

class DescriptorDataAppendAllocator {
public:
    DescriptorDataAppendAllocator(DeviceDispatchTable* table, const ComRef<DeviceAllocator>& allocator, ShaderExportRenderPassState* renderPass, size_t maxChunkSize) : maxChunkSize(maxChunkSize), allocator(allocator), renderPass(renderPass), table(table) {

    }

    /// Set the chunk
    /// \param commandBuffer upload command buffer
    /// \param segmentEntry segment to be bound to
    void SetChunk(VkCommandBuffer commandBuffer, const DescriptorDataSegmentEntry& segmentEntry) {
        // Set inherited width
        chunkSize = segmentEntry.width / sizeof(uint32_t);

        // Add entry
        segment.entries.emplace_back(segmentEntry);

        // Set mapped
        mapped = static_cast<uint32_t*>(allocator->Map(segmentEntry.allocation.host));

        // Clear mapped data
        std::memset(mapped, 0x0, segmentEntry.width);

        // Guard against render passes
        CommandBufferRenderPassScope renderPassScope(table, commandBuffer, renderPass);

        // Copy host to device
        VkBufferCopy copy;
        copy.srcOffset = 0;
        copy.dstOffset = 0;
        copy.size = segmentEntry.width;
        table->commandBufferDispatchTable.next_vkCmdCopyBuffer(commandBuffer, segmentEntry.bufferHost, segmentEntry.bufferDevice, 1u, &copy);

        // Transfer to shader barrier
        VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = segmentEntry.bufferDevice;
        barrier.size = segmentEntry.width;
        barrier.offset = 0u;
        table->commandBufferDispatchTable.next_vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0x0,
            0, nullptr,
            1u, &barrier,
            0, nullptr
        );
    }

    /// Begin a new segment
    /// \param rootCount number of root parameters
    void BeginSegment(uint32_t rootCount, bool migrateData) {
        migrateLastSegment = migrateData;
        pendingRootCount = rootCount;
        pendingRoll = true;
    }

    /// Set a root value
    /// \param commandBuffer upload command buffer
    /// \param offset current root offset
    /// \param value value at root offset
    void Set(VkCommandBuffer commandBuffer, uint32_t offset, uint32_t value) {
        if (pendingRoll) {
            RollChunk(commandBuffer);
        }

        ASSERT(offset < mappedSegmentLength, "Out of bounds descriptor segment offset");
        mapped[mappedOffset + offset] = value;
    }

    /// Set a root value
    /// \param commandBuffer upload command buffer
    /// \param offset current root offset
    /// \param value value at root offset
    void SetOrAllocate(VkCommandBuffer commandBuffer, uint32_t offset, uint32_t allocationSize, uint32_t value) {
        // Begin a new segment if the previous does not suffice, may be allocated dynamically 
        if (offset >= mappedSegmentLength || (pendingRoll && allocationSize >= pendingRootCount)) {
            ASSERT(allocationSize > offset, "Chunk allocation size must be larger than the expected offset");
            BeginSegment(allocationSize, mappedSegmentLength == allocationSize);
        }

        // Roll! D2! (Never played DnD, sorry)
        if (pendingRoll) {
            RollChunk(commandBuffer);
        }

        ASSERT(offset < mappedSegmentLength, "Chunk allocation failed");
        mapped[mappedOffset + offset] = value;
    }
    
    /// Manually roll the chunk
    void ConditionalRoll(VkCommandBuffer commandBuffer) {
        if (pendingRoll) {
            RollChunk(commandBuffer);
        }
    }

    /// Has this allocator been rolled? i.e. a new segment has begun
    /// \return
    bool HasRolled() const {
        return !pendingRoll;
    }

    /// Commit all changes for the GPU
    void Commit() {
        if (!mapped) {
            return;
        }

        // Get entry
        DescriptorDataSegmentEntry& entry = segment.entries.back();

        // Unmap range
        allocator->Unmap(entry.allocation.host);
        mapped = nullptr;
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

    /// Validate this append allocator
    void ValidateReleased() {
        ASSERT(chunkSize == 0 && mapped == nullptr, "Unexpected state");
    }

private:
    /// Roll the current chunk
    /// \param commandBuffer upload command buffer
    void RollChunk(VkCommandBuffer commandBuffer) {
        // Advance current offset
        uint64_t nextMappedOffset = mappedOffset + mappedSegmentLength;

        // Out of memory?
        if (nextMappedOffset + pendingRootCount >= chunkSize) {
            // Copy last chunk if migration is requested
            uint32_t* lastChunkDwords = ALLOCA_ARRAY(uint32_t, mappedSegmentLength);
            if (migrateLastSegment) {
                std::memcpy(lastChunkDwords, mapped + mappedOffset, sizeof(uint32_t) * mappedSegmentLength);
            }

            // Growth factor of 1.5
            chunkSize = std::max<size_t>(64'000, static_cast<size_t>(chunkSize * 1.5f));

            // Account for device limits
            chunkSize = std::min<size_t>(chunkSize, maxChunkSize / sizeof(uint32_t));

            // Create new chunk
            uint64_t lastSegmentLength = mappedSegmentLength;
            CreateChunk(commandBuffer);

            // Migrate last segment?
            if (migrateLastSegment) {
                size_t count = std::min<size_t>(pendingRootCount, lastSegmentLength);
                std::memcpy(mapped + nextMappedOffset, lastChunkDwords, sizeof(uint32_t) * count);
            }
        } else {
            // Migrate last segment?
            if (migrateLastSegment) {
                size_t count = std::min<size_t>(pendingRootCount, mappedSegmentLength);
                std::memcpy(mapped + nextMappedOffset, mapped + mappedOffset, sizeof(uint32_t) * count);
            }
            
            // Set new offset
            mappedOffset = nextMappedOffset;
        }

        // Set next roll length
        mappedSegmentLength = pendingRootCount;
        pendingRoll = false;
        migrateLastSegment = false;
    }

    /// Create a new chunk
    /// \param commandBuffer upload command buffer
    void CreateChunk(VkCommandBuffer commandBuffer) {
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
        SetChunk(commandBuffer, segmentEntry);
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
    
    /// Should the last segment be migrated on rolls?
    bool migrateLastSegment{false};

    /// Device allocator
    ComRef<DeviceAllocator> allocator;

    /// Streaming render pass state
    ShaderExportRenderPassState* renderPass;

    /// Parent table
    DeviceDispatchTable* table;

private:
    /// Current data segment, to be released later
    DescriptorDataSegment segment;

    /// Current mapped address of the segment
    uint32_t* mapped{nullptr};
};
