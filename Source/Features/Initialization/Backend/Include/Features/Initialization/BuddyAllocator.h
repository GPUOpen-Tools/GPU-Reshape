// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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

// Feature
#include <Features/Initialization/BuddyAllocation.h>

// Common
#include <Common/Assert.h>

// Std
#include <bit>
#include <vector>

class BuddyAllocator {    
public:
    static constexpr uint32_t kMaxLevels = 34;
    
    /// Install this allocator
    /// \param size total byte size requested
    void Install(uint64_t size) {
        // If not aligned to two, use the previous power of two
        if ((size & (size - 1)) != 0) {
            size = 1ull << (std::bit_width(size) - 1);
        }

        // Allocate a root node for the entire length
        uint32_t rootLevel = GetLevel(size);
        uint32_t rootNode  = AllocateNode(kInvalidNode, 0ull, rootLevel);

        // Mark the root node as free
        PushFree(rootLevel, rootNode);
    }

    /// Allocate a region
    /// \param length byte length
    /// \return kInvalidBuddyAllocation if failed
    BuddyAllocation Allocate(uint64_t length) {
        // Get the lowest level for this length
        uint32_t lowLevel = GetLevel(length);

        // Just because we want a certain level, doesn't mean one's free yet
        uint32_t availableLevel = FindFirstAvailableLevel(lowLevel);

        // If no levels have free nodes, we're out of space (internal fragmentation or not)
        if (availableLevel == kInvalidLevel) {
            return kInvalidBuddyAllocation;
        }

        // Pop a free node from the first available level
        uint32_t nodeIndex = PopFree(availableLevel);

        // Traverse down to the level, subdividing as needed
        while (availableLevel != lowLevel) {
            // Subdivide this node
            if (nodes[nodeIndex].lhs == kInvalidNode) {
                uint32_t nextLevel = availableLevel - 1;

                // Create two children, half width's
                nodes[nodeIndex].lhs = AllocateNode(nodeIndex, nodes[nodeIndex].offset, nextLevel);
                nodes[nodeIndex].rhs = AllocateNode(nodeIndex, nodes[nodeIndex].offset + (1ull << nextLevel),  nextLevel);
            }

            // Mark RHS as free
            PushFree(availableLevel - 1, nodes[nodeIndex].rhs);

            // Continue travesal from LHS
            nodeIndex = nodes[nodeIndex].lhs;

            // Next!
            availableLevel--;
        }

        // Just return the node's offset, ignore padding
        ASSERT(length <= 1ull << nodes[nodeIndex].level, "Invalid node");
        return BuddyAllocation {
            .offset = nodes[nodeIndex].offset,
            .nodeIndex = nodeIndex
        };
    }

    /// Free an allocation
    /// \param allocation given allocation
    void Free(const BuddyAllocation& allocation) {
        ASSERT(nodes[allocation.nodeIndex].lhs == kInvalidNode, "Expected leaf node");
        FreeNodeRecursive(allocation.nodeIndex);
    }

private:
    /// Invalid indices
    static constexpr uint32_t kInvalidLevel = UINT32_MAX;
    static constexpr uint32_t kInvalidNode = UINT32_MAX;
    static constexpr uint32_t kInvalidSlot = UINT32_MAX;
    
    struct Node {
        /// Byte offset of this node
        uint64_t offset = 0;

        /// Level (2^n) of this node
        uint32_t level = 0;

        /// Parent node, if any
        uint32_t parentNode = kInvalidNode;

        /// If part of a free list, its slot
        uint32_t freeSlot = kInvalidSlot;

        /// Child nodes, invalid on leaf nodes
        uint32_t lhs = kInvalidNode;
        uint32_t rhs = kInvalidNode;
    };
    
    struct LevelEntry {
        /// All free nodes in this level
        std::vector<uint32_t> freeNodes;
    };

    /// Pop a free allocation
    /// \param level target level
    /// \return kInvalidNode if not found
    uint32_t PopFree(uint32_t level) {
        LevelEntry& entry = levels[level];

        // None at all?
        if (entry.freeNodes.empty()) {
            return kInvalidNode;
        }

        // Pop node
        uint32_t index = entry.freeNodes.back();
        entry.freeNodes.pop_back();

        // Cleanup
        nodes[index].freeSlot = kInvalidSlot;
        return index;
    }

    /// Push a free allocation
    /// \param level target level
    /// \param nodeIndex index to push
    void PushFree(uint32_t level, uint32_t nodeIndex) {
        // Set the free slot of this node
        nodes[nodeIndex].freeSlot = static_cast<uint32_t>(levels[level].freeNodes.size());

        // Add to array
        levels[level].freeNodes.push_back(nodeIndex);
    }

    /// Allocate a new node, ideally during exhaustion
    /// \param parentNode parent node, if any
    /// \param offset byte offset
    /// \param level target level
    /// \return node index
    uint32_t AllocateNode(uint32_t parentNode, uint64_t offset, uint32_t level) {
        uint32_t index;

        // Reuse index if possible
        if (freeNodeIndices.empty()) {
            index = static_cast<uint32_t>(nodes.size());
            nodes.emplace_back();
        } else {
            index = freeNodeIndices.back();
            freeNodeIndices.pop_back();
        }

        // Setup
        nodes[index].offset = offset;
        nodes[index].level = level;
        nodes[index].parentNode = parentNode;
        return index;
    }

    /// Remove a node from the free array
    /// \param nodeIndex target node to remove
    void RemoveFromFree(uint32_t nodeIndex) {
        Node& node = nodes[nodeIndex];

        // Current level
        LevelEntry& level = levels[node.level];

        // If this node is not the last free node
        if (node.freeSlot != level.freeNodes.size() - 1) {
            uint32_t lastFreeNode = level.freeNodes.back();

            // Move the actual last node to the current position
            // Reassign its free slot index
            level.freeNodes[node.freeSlot] = lastFreeNode;
            nodes[lastFreeNode].freeSlot = node.freeSlot;
        }

        // Pop the last one, guaranteed to be ours
        level.freeNodes.pop_back();

        // No longer part of any list
        node.freeSlot = kInvalidSlot;
    }

    /// Free a node and merge its parents, if possible
    /// \param nodeIndex node to free
    void FreeNodeRecursive(uint32_t nodeIndex) {
        // Destroy the children
        if (nodes[nodeIndex].lhs != kInvalidNode) {
            DestroyFreeNode(nodes[nodeIndex].lhs);
            DestroyFreeNode(nodes[nodeIndex].rhs);
        }

        // Mark this node as free
        PushFree(nodes[nodeIndex].level, nodeIndex);

#if 0 // todo[init]: This is broken!
        // If the parent node is fully free, merge it upwards
        if (uint32_t parentNode = nodes[nodeIndex].parentNode; parentNode != kInvalidNode) {
            if (IsFree(nodes[parentNode].lhs) && IsFree(nodes[parentNode].rhs)) {
                // Free the parent node
                FreeNodeRecursive(parentNode);
            }
        }
#endif
    }

    /// Destroy a node
    /// \param nodeIndex node to destroy 
    void DestroyFreeNode(uint32_t nodeIndex) {
        ASSERT(IsFree(nodeIndex), "Unexpected state");
    
        // Remove from its pending free list
        RemoveFromFree(nodeIndex);

        // Reset node data
        nodes[nodeIndex] = {};

        // Mark as free
        freeNodeIndices.push_back(nodeIndex);
    }

    /// Check if a node is free
    /// \param nodeIndex node to check
    /// \return true if free
    bool IsFree(uint32_t nodeIndex) {
        return nodes[nodeIndex].freeSlot != kInvalidSlot;
    }

    /// Find the first available level from the lowest level
    /// \param lowLevel the target lowest level
    /// \return first available level
    uint32_t FindFirstAvailableLevel(uint32_t lowLevel) {
        for(; lowLevel < kMaxLevels; lowLevel++) {
            LevelEntry& entry = levels[lowLevel];
            if (!entry.freeNodes.empty()) {
                return lowLevel;
            }
        }

        // None found, out of allocations entirely
        return kInvalidLevel;
    }

    /// Get the level from a width
    /// \param width width of the allocation
    /// \return level
    uint32_t GetLevel(uint64_t width) {
        ASSERT(width < (1ull << kMaxLevels), "Out of levels");
        return std::bit_width(std::max(1ull, width - 1u));
    }

private:
    /// All tiles
    std::vector<uint32_t> tileResidency;

    /// All nodes, may be sparsely populated
    std::vector<Node> nodes;

    /// All levels
    LevelEntry levels[kMaxLevels];

    /// Free node indices for later reuse
    std::vector<uint32_t> freeNodeIndices;
};
