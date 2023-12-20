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
#include "RelocationOffset.h"

// Common
#include <Common/Allocators.h>

// Std
#include <cstdint>
#include <vector>

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
            RelocationOffset indices[kBlockSize]{};
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
