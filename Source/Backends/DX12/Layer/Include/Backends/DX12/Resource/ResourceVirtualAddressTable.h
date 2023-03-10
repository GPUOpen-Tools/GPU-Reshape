#pragma once

// Common
#include <Common/Allocator/BTree.h>

// Std
#include <mutex>

// Forward declarations
struct ResourceState;

class ResourceVirtualAddressTable {
public:
    /// Constructor
    ResourceVirtualAddressTable(const Allocators &allocators) : entries(allocators) {

    }

    /// Add a new address mapping
    /// \param state given state
    /// \param base base address
    /// \param length address length
    void Add(ResourceState *state, uint64_t base, uint64_t length) {
        std::lock_guard guard(lock);
        entries[base] = AddressEntry{
            .state = state,
            .length = length
        };
    }

    /// Remove an address
    /// \param base base address
    void Remove(uint64_t base) {
        std::lock_guard guard(lock);
        entries.erase(base);
    }

    /// Find the resource for a given address
    /// \param offset given virtual address
    /// \return nullptr if not found
    ResourceState *Find(uint64_t offset) {
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
        if (offset - it->first > it->second.length) {
            return nullptr;
        }

        // OK
        return it->second.state;
    }

private:
    struct AddressEntry {
        ResourceState *state{nullptr};
        uint64_t length{0};
    };

    /// Shared lock
    std::mutex lock;

    /// All virtual addresses tracked
    BTreeMap<uint64_t, AddressEntry> entries;
};
