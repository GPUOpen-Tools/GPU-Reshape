#pragma once

#include <cstdint>

// Backend
#include "RelocationOffset.h"

// Common
#include <Common/Allocators.h>

namespace IL {
    /// Block relocation allocator, for safe references to contiguous instruction streams
    class RelocationAllocator {
    public:
        /// Constructor
        RelocationAllocator(const Allocators& allocators) : allocators(allocators) {

        }

        /// Destructor
        ~RelocationAllocator() {
            for (RelocationAddressBlock* block : blocks) {
                destroy(block, allocators);
            }
        }

        /// Allow move
        RelocationAllocator(RelocationAllocator&& other) = default;

        /// No copy
        RelocationAllocator(const RelocationAllocator& other) = delete;

        /// No copy assignment
        RelocationAllocator& operator=(const RelocationAllocator& other) = delete;
        RelocationAllocator& operator=(RelocationAllocator&& other) = delete;

        /// Allocate a new relocation offset
        /// \return relocation offset
        RelocationOffset* Allocate() {
            // Reuse existing index
            if (!freeIndices.empty()) {
                RelocationOffset* index = freeIndices.back();
                freeIndices.pop_back();
                return index;
            }

            // Try existing blocks
            for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
                RelocationAddressBlock* block = *it;

                if (block->head < kBlockSize) {
                    RelocationOffset* index = &block->indices[block->head];
                    block->head++;
                    return index;
                }
            }

            // Allocate new block
            auto* block = new(allocators) RelocationAddressBlock;
            block->head = 1;
            blocks.push_back(block);

            return &block->indices[0];
        }

        /// Free a relocation offset
        /// \param index the offset to be freed, invalid after call
        void Free(RelocationOffset* index) {
            freeIndices.push_back(index);
        }

    private:
        static constexpr uint32_t kBlockSize = 64;

        /// Allocation block
        struct RelocationAddressBlock {
            RelocationOffset indices[kBlockSize];
            uint32_t head{0};
        };

        /// All allocated blocks
        std::vector<RelocationAddressBlock *> blocks;

        /// Free indices
        std::vector<RelocationOffset *> freeIndices;

    private:
        Allocators allocators;
    };
};
