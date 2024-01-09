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

// Common
#include <Common/Allocators.h>
#include <Common/Assert.h>

// Std
#include <cstdint>
#include <vector>

template<size_t BLOCK_SIZE>
struct LinearBlockAllocator {
    /// Constructor
    LinearBlockAllocator(const Allocators& allocators = {}) : allocators(allocators) {

    }

    /// Destructor
    ~LinearBlockAllocator() {
        Clear();
    }

    /// Allocate a type, type must be trivially destructible
    /// \param args constructor arguments
    /// \return the allocated type
    template<typename T, typename... A>
    T* Allocate(A&&... args) {
        ASSERT(sizeof(T) <= BLOCK_SIZE, "Allocation larger than block size");

        // Any previous blocks?
        for (; freeBlockHead < blocks.size(); freeBlockHead++) {
            Block& block = blocks[freeBlockHead];

            // Enough space to accomodate?
            if (sizeof(T) + block.head <= block.tail) {
                size_t offset = block.head;
                block.head += sizeof(T);

                // Allocate in place
                return new (block.data + offset) T(args...);
            }
        }

        // Push new block
        Block& block = AllocateBlock();
        block.head = sizeof(T);

        // Allocate in place
        return new (block.data) T(args...);
    }

    /// Allocate a type, type must be trivially destructible
    /// \param count the number of elements
    /// \return the allocated type
    template<typename T>
    T* AllocateArray(uint32_t count) {
        const size_t size = sizeof(T) * count;

        // Allow for unexpected sizes on arrays
        if (size > BLOCK_SIZE) {
            T* alloc = new (allocators) T[count];
            freeAllocations.push_back(alloc);
            return alloc;
        }

        // Any previous blocks?
        for (; freeBlockHead < blocks.size(); freeBlockHead++) {
            Block& block = blocks[freeBlockHead];

            // Enough space to accomodate?
            if (size + block.head <= block.tail) {
                size_t offset = block.head;
                block.head += size;

                // No alloc
                return reinterpret_cast<T*>(block.data + offset);
            }
        }
        
        // Push new block
        Block& block = AllocateBlock();
        block.head = size;

        // No alloc
        return reinterpret_cast<T*>(block.data);
    }

    /// Clear this allocator, free all blocks
    void Clear() {
        for (Block& block : blocks) {
            destroy(block.data, allocators);
        }

        for (void* _free : freeAllocations) {
            allocators.free(allocators.userData, _free, kDefaultAlign);
        }

        blocks.clear();
    }

    /// Clear this allocator, keep blocks alive
    void ClearSubAllocations() {
        // Note: This ... is not entirely correct. Allocations can happen at varying sizes, so we could run into a case
        //       where the visitation will skip valid blocks for future candidates. But, I believe this is not the purpose
        //       of this allocator anyway.
        freeBlockHead = 0;
        
        for (Block& block : blocks) {
            block.head = 0;
        }

        for (void* _free : freeAllocations) {
            allocators.free(allocators.userData, _free, kDefaultAlign);
        }
    }

    /// Swap this container
    /// \param rhs container to swap with
    void Swap(LinearBlockAllocator<BLOCK_SIZE>& rhs) {
        std::swap(freeBlockHead, rhs.freeBlockHead);
        blocks.swap(rhs.blocks);
        freeAllocations.swap(rhs.freeAllocations);
    }

private:
    struct Block {
        uint8_t* data{nullptr};
        size_t head{0};
        size_t tail{0};
    };

    /// Allocate a new block
    Block& AllocateBlock() {
        constexpr float kGrowthFactor = 1.5f;

        // New tail
        const size_t length = blocks.empty() ? BLOCK_SIZE : static_cast<size_t>(blocks.back().tail * kGrowthFactor);
        
        // Create block
        Block& block = blocks.emplace_back();
        block.tail = length;
        block.data = new (allocators) uint8_t[block.tail];
        return block;
    }

private:
    Allocators allocators;

    /// All allocated blocks
    std::vector<Block> blocks;

    /// Current block visitation head
    uint32_t freeBlockHead{0};

    /// Loose allocations, free'd manually
    std::vector<void*> freeAllocations;
};