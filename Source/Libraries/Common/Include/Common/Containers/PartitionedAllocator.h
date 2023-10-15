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

// Common
#include <Common/Assert.h>

// Std
#include <algorithm>
#include <vector>

/// Invalid partition block, out of space
static constexpr uint32_t kInvalidPartitionBlock = ~0u;

/// Partitioned allocator, useful for reducing fragmentation on allocations with known lengths
/// \tparam PARTITION_COUNT Number of partition levels, each level being 2 ^ N
/// \tparam PARTITION_SIZE Size of each partition, effective block count driven by partition level
/// \tparam LARGE_PARTITION_SLACK Slack on large allocations, large allocations are reused if in the bounds of +/- SLACK
template<size_t PARTITION_COUNT, size_t PARTITION_SIZE, size_t LARGE_PARTITION_SLACK>
class PartitionedAllocator {
public:
    PartitionedAllocator() {
        // Create all partition metadatas
        for (size_t i = 0; i < PARTITION_COUNT; i++) {
            PartitionMetadata& metadata = partitionMetadata[i];
            metadata.blockLength = (1ull << i);
            metadata.blockCount = PARTITION_SIZE / metadata.blockLength;
        }
    }

    /// Set the length of the allocator
    /// \param length Total length
    void SetLength(size_t length) {
        regionLength = length;
    }

    /// Allocate a new block
    /// \param count number of elements
    /// \return block offset, kInvalidPartitionBlock if failed
    size_t Allocate(uint32_t count) {
        return AllocatePartitionBlock(count).offset;
    }

    /// Free a block
    /// \param offset block offset 
    /// \param count number of elements, must be the same as the allocation count
    void Free(size_t offset, uint32_t count) {
        PartitionBlock block;
        block.offset = offset;
        FreePartitionBlock(block, count);
    }
    
private:
    /// Single block
    struct PartitionBlock {
        size_t offset{~0u};
    };

    /// Slack block for large allocations
    struct SlackPartitionBlock : PartitionBlock {
        size_t length{~0u};

        /// Comparator for sorting
        bool operator<(const SlackPartitionBlock& rhs) const {
            return length < rhs.length;
        }
    };
    
    struct PartitionMetadata {
        /// Number of blocks per partition
        size_t blockCount{~0u};

        /// Number of elements per block
        size_t blockLength{~0u};

        /// All free blocks
        std::vector<PartitionBlock> freeBlocks;
    };

private:
    /// Get the partition level
    /// \param count number of elements
    /// \return partition level
    unsigned long GetPartitionLevel(uint32_t count) {
        // Find the first set MSB
        unsigned long partitionLevel;
        ENSURE(_BitScanReverse(&partitionLevel, count), "Zero length allocations are not supported");

        // Select next level if there's any pending bits
        if (count & ~(1u << partitionLevel)) {
            partitionLevel++;
        }

        // OK
        return partitionLevel;
    }

    /// Allocate a new partition block
    /// \param count number of elements
    /// \return partition block
    PartitionBlock AllocatePartitionBlock(uint32_t count) {
        unsigned long partitionLevel = GetPartitionLevel(count);
        
        // Large partition?
        if (partitionLevel >= PARTITION_COUNT) {
            return AllocateSlackPartitionBlock(count);
        }

        // Get metadata
        PartitionMetadata& metadata = partitionMetadata[partitionLevel];

        // Validate bounds
#ifndef NDEBUG
        if (count > metadata.blockLength || (partitionLevel != 0 && count <= partitionMetadata[partitionLevel - 1].blockLength)) {
            ASSERT(false, "Invalid parition metadata");
        }
#endif // NDEBUG

        // Out of free blocks?
        if (metadata.freeBlocks.empty()) {
            return AllocatePartitionBlockWithLeading(metadata);
        }

        // Must have a block at this point
        ASSERT(!metadata.freeBlocks.empty(), "Partition allocation failed");

        // Assume last
        PartitionBlock partition = metadata.freeBlocks.back();
        metadata.freeBlocks.pop_back();
        return partition;
    }

    /// Free a partition block
    /// \param block block to be free'd
    /// \param count number of elements, must be the allocation count
    void FreePartitionBlock(const PartitionBlock& block, uint32_t count) {
        unsigned long partitionLevel = GetPartitionLevel(count);

        // Large partition?
        if (partitionLevel >= PARTITION_COUNT) {
            FreeSlackPartitionBlock(block, count);
            return;
        }

        // Get metadata
        PartitionMetadata& metadata = partitionMetadata[partitionLevel];

        // Mark as free
        metadata.freeBlocks.push_back(block);
    }

    /// Free a slack partition block
    /// \param block block to be free'd
    /// \param count number of elements, must be the allocation count
    void FreeSlackPartitionBlock(const PartitionBlock& block, uint32_t count) {
        size_t slackHighBound = count + LARGE_PARTITION_SLACK;

        // Find sorted insertion point [slackedSize, end)
        auto it = std::upper_bound(freeSlackBlocks.begin(), freeSlackBlocks.end(), SlackPartitionBlock {
            .length = slackHighBound
        });

        // Mark as free
        SlackPartitionBlock largeBlock;
        largeBlock.offset = block.offset;
        largeBlock.length = slackHighBound;
        freeSlackBlocks.insert(it, largeBlock);
    }

    /// Allocate a partition block, and acquire the leading block
    /// \param metadata partition level to allocate for
    /// \return partition block
    PartitionBlock AllocatePartitionBlockWithLeading(PartitionMetadata& metadata) {
        size_t elementCount = metadata.blockLength * metadata.blockCount;

        // Out of space?
        if (regionOffset + elementCount > regionLength) {
            return {kInvalidPartitionBlock};
        }

        // Assign at current offset
        PartitionBlock leadingBlock;
        leadingBlock.offset = regionOffset;

        // Advance region
        regionOffset += elementCount;

        // Create remaining blocks
        for (size_t i = 1; i < metadata.blockCount; i++) {
            PartitionBlock freeBlock;
            freeBlock.offset = leadingBlock.offset + metadata.blockLength * i;
            metadata.freeBlocks.push_back(freeBlock);
        }

        // OK
        return leadingBlock;
    }

    /// Allocate a slack partition block
    /// \param count number of elements
    /// \return partition block
    PartitionBlock AllocateSlackPartitionBlock(uint32_t count) {
        size_t slackHighBound = count + LARGE_PARTITION_SLACK;

        // Out of space?
        if (regionOffset + slackHighBound > regionLength) {
            return{ kInvalidPartitionBlock };
        }

        // Find sorted query point [slackHighBound, end)
        auto it = std::upper_bound(freeSlackBlocks.begin(), freeSlackBlocks.end(), SlackPartitionBlock{.length = count - 1u});

        // Appropriate block? (DistToEdge <= Slack * 2u) 
        if (it != freeSlackBlocks.end() && it->length >= count && it->length - count <= LARGE_PARTITION_SLACK * 2u) {
            PartitionBlock block = *it;
            freeSlackBlocks.erase(it);
            return block;
        }

        // Allocate at offset
        PartitionBlock block;
        block.offset = regionOffset;

        // Advance region
        regionOffset += slackHighBound;

        // OK
        return block;
    }

    /// All partition levels
    PartitionMetadata partitionMetadata[PARTITION_COUNT];

    /// Free slack partition blocks
    std::vector<SlackPartitionBlock> freeSlackBlocks;

private:
    /// Current region offset
    size_t regionOffset{0};

    /// Total region length (user driven)
    size_t regionLength{0};
};
