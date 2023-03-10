#pragma once

// Common
#include <Common/Allocator/BTree.h>

// Std
#include <mutex>

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
        std::lock_guard guard(lock);
        entries[base] = HeapEntry {
            .count = count,
            .stride = stride,
            .heap = heap
        };
    }

    /// Remove a heap from tracking
    /// \param base base descriptor offset
    void Remove(uint64_t base) {
        std::lock_guard guard(lock);
        entries.erase(base);
    }

    /// Find a given heap
    /// \param offset descriptor offset
    /// \return nullptr if not found
    DescriptorHeapState* Find(uint64_t offset) {
        std::lock_guard guard(lock);
        if (entries.empty()) {
            return nullptr;
        }

        // Sorted search
        auto it = --entries.upper_bound(offset);

        // Bad lower?
        if (offset < it->first) {
            return nullptr;
        }
        
        // Validate against upper
        if (offset - it->first > it->second.count * it->second.stride) {
            return nullptr;
        }

        // OK
        return it->second.heap;
    }

private:
    struct HeapEntry {
        uint64_t count{0};
        uint64_t stride{0};
        DescriptorHeapState* heap{nullptr};
    };

    /// Shared lock
    std::mutex lock;

    /// All heaps tracked
    BTreeMap<uint64_t, HeapEntry> entries;
};
