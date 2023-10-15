// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
