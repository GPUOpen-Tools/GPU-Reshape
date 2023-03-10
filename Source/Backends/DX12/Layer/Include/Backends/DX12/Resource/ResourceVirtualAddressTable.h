#pragma once

// Common
#include <Common/Allocator/BTree.h>

// Forward declarations
struct ResourceState;

class ResourceVirtualAddressTable {
public:
    /// Constructor
    ResourceVirtualAddressTable(const Allocators &allocators) : entries(allocators) {

    }

    /// Add a new entry
    /// \param state given state
    /// \param base base address
    /// \param length address length
    void Add(ResourceState *state, uint64_t base, uint64_t length) {
        entries[base] = AddressEntry{
            .state = state,
            .length = length
        };
    }

    /// Remove an entry
    /// \param base base address
    void Remove(uint64_t base) {
        entries.erase(base);
    }

    /// Find the resource for a given address
    /// \param offset given virtual address
    /// \return nullptr if not found
    ResourceState *Find(uint64_t offset) const {
        auto it = entries.upper_bound(offset);
        if (it == entries.end()) {
            return nullptr;
        }

        --it;

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
        ResourceState *state;
        uint64_t length;
    };

    /// All virtual addresses tracked
    BTreeMap<uint64_t, AddressEntry> entries;
};
