#pragma once

// Common
#include <Common/Allocator/Vector.h>

// Std
#include <vector>
#include <algorithm>

// Forward declarations
struct DescriptorHeapState;

class HeapTable {
public:
    HeapTable(const Allocators& allocators) : entries(allocators) {
        
    }
    
    /// Add a new heap for tracking
    /// \param heap heap address
    /// \param base base descriptor offset
    /// \param count number of descriptors
    /// \param stride stride of each descriptor
    void Add(DescriptorHeapState* heap, uint64_t base, uint64_t count, uint64_t stride) {
        HeapEntry entry {
            .base = base,
            .count = count,
            .stride = stride,
            .heap = heap
        };

        // Sorted insertion
        entries.insert(std::upper_bound(entries.begin(), entries.end(), entry), entry);
    }

    /// Find a given heap
    /// \param offset descriptor offset
    /// \return nullptr if not found
    DescriptorHeapState* Find(uint64_t offset) const {
        if (entries.empty()) {
            return nullptr;
        }

        // Sorted search
        auto it = --std::lower_bound(entries.begin(), entries.end(), HeapEntry {.base = offset});

        // May not be valid
        if (it->base + it->count * it->stride <= offset) {
            return nullptr;
        }

        return it->heap;
    }

private:
    struct HeapEntry {
        bool operator<(const HeapEntry& rhs) const {
            return base <= rhs.base;
        }

        uint64_t base{0};
        uint64_t count{0};
        uint64_t stride{0};
        DescriptorHeapState* heap{nullptr};
    };

    /// All heaps tracked
    Vector<HeapEntry> entries;
};
