#pragma once

// Common
#include "Allocators.h"
#include "Assert.h"

// Std
#include <cstdint>
#include <vector>

template<size_t BLOCK_SIZE>
struct LinearBlockAllocator {
    /// Constructor
    LinearBlockAllocator(const Allocators& allocators) : allocators(allocators) {

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

        // Check existing blocks
        for (auto it = blocks.rbegin(); it != blocks.rend(); it++) {
            Block* block = *it;

            if (BLOCK_SIZE - block->head <= sizeof(T)) {
                size_t offset = block->head;
                block->head += sizeof(T);

                return new (&block->data[offset]) T(args...);
            }
        }

        // Push new block
        Block* block = new (allocators) Block();
        block->head = sizeof(T);
        blocks.push_back(block);

        return new (&block->data[0]) T(args...);
    }

    /// Clear this allocator, free all blocks
    void Clear() {
        for (Block* block : blocks) {
            destroy(block, allocators);
        }

        blocks.clear();
    }

private:
    Allocators allocators;

    struct Block {
        uint8_t data[BLOCK_SIZE];
        size_t head{0};
    };

    // All allocated blocks
    std::vector<Block*> blocks;
};