#pragma once

// Common
#include <Common/Allocator/BTree.h>

// Std
#include <mutex>

// Forward declarations
struct DescriptorHeapState;

class HeapTable {
public:
    HeapTable(const Allocators& allocators) : alignmentBuckets(allocators), allocators(allocators) {
        
    }

    /// Set the stride bound
    /// \param stride maximum stride
    void SetStrideBound(uint32_t stride) {
        for (uint32_t i = 0; i < stride; i++) {
            alignmentBuckets.emplace_back(allocators);
        }
    }
    
    /// Add a new heap for tracking
    /// \param heap heap address
    /// \param base base descriptor offset
    /// \param count number of descriptors
    /// \param stride stride of each descriptor
    void Add(DescriptorHeapState* heap, uint64_t base, uint64_t count, uint64_t stride) {
        std::lock_guard guard(lock);
        GetAlignmentBucket(base).entries[base] = HeapEntry {
            .count = count,
            .stride = stride,
            .heap = heap
        };
    }

    /// Remove a heap from tracking
    /// \param base base descriptor offset
    void Remove(uint64_t base) {
        std::lock_guard guard(lock);
        GetAlignmentBucket(base).entries.erase(base);
    }

    /// Find a given heap
    /// \param offset descriptor offset
    /// \return nullptr if not found
    DescriptorHeapState* Find(uint64_t offset) {
        std::lock_guard guard(lock);

        HeapAlignmentBucket& bucket = GetAlignmentBucket(offset);

        if (bucket.entries.empty()) {
            return nullptr;
        }

        // Sorted search
        auto it = --bucket.entries.upper_bound(offset);

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
        /// Number of descriptors in this heap
        uint64_t count{0};

        /// Given stride of the heap
        uint64_t stride{0};

        /// Underlying heap
        DescriptorHeapState* heap{nullptr};
    };

    struct HeapAlignmentBucket {
        HeapAlignmentBucket(const Allocators& allocators) : entries(allocators) {

        }

        /// All heaps tracked in this bucket
        BTreeMap<uint64_t, HeapEntry> entries;
    };

    /// Get the owning bucket of an offset
    /// \param offset opaque offset
    /// \return owning bucket
    HeapAlignmentBucket& GetAlignmentBucket(uint64_t offset) {
        return alignmentBuckets.at(offset % alignmentBuckets.size());
    }

    /// Linear buckets
    Vector<HeapAlignmentBucket> alignmentBuckets;

private:
    Allocators allocators;

    /// Shared lock
    std::mutex lock;
};
