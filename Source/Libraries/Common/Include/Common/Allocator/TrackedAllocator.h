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
#include <Common/Allocators.h>

// Std
#include <unordered_map>
#include <ostream>
#include <mutex>

class TrackedAllocator {
public:
    TrackedAllocator() {
        // Create untagged entry
        auto&& _default = taggedEntries[0];
        _default.name = "Default";
    }
    
    void* Allocate(size_t size, size_t align, AllocationTag tag) {
        // Scoped
        {
            std::lock_guard guard(mutex);
        
            // Find entry or create
            auto it = taggedEntries.find(tag.crc64);
            if (it == taggedEntries.end()) {
                // Allocate entry
                it = taggedEntries.emplace(tag.crc64, MappedEntry{}).first;

                // Set name
                it->second.name = tag.name;
            }

            // Track
            it->second.count++;
            it->second.length += static_cast<int64_t>(size);
        }

        const size_t alignedOffset = AlignedOffset(align);

        // Allocate with tagged header
        auto* data = static_cast<Header*>(malloc(alignedOffset + size));
        data->tag = tag.crc64;
        data->size = size;

        // Return head
        return reinterpret_cast<uint8_t*>(data) + alignedOffset;
    }
    
    void Free(void* head, size_t align) {
        const size_t alignedOffset = AlignedOffset(align);
        
        // Get header
        auto* data = reinterpret_cast<Header*>(static_cast<uint8_t*>(head) - alignedOffset);

        // Scoped
        {
            std::lock_guard guard(mutex);

            // Track
            MappedEntry& entry = taggedEntries.at(data->tag);
            entry.length -= static_cast<int64_t>(data->size);
        }

        // Finally, free the data
        free(data);
    }

    void Dump(std::ostream& out) {
        std::lock_guard guard(mutex);
        out << "TrackedAllocator\n";

        // Simple punctuator
        struct Punctuator : std::numpunct<char> {
            char_type do_thousands_sep() const override { return '\''; }
            string_type do_grouping() const override { return "\3"; }
        };

        // Previous locale
        const std::locale restore = out.getloc();

        // Set punctuated locale
        out.imbue(std::locale(out.getloc(), std::make_unique <Punctuator>().release()));

        // Sort entries
        std::vector<MappedEntry> sortedEntries;
        for (std::pair<const unsigned long long, MappedEntry> &it: taggedEntries) {
            sortedEntries.insert(std::ranges::lower_bound(sortedEntries, it.second, [](const MappedEntry& lhs, const MappedEntry& rhs) {
                return lhs.length > rhs.length;
            }), it.second);
        }

        // Default padding
        constexpr uint32_t kPadding = 45;

        // Total byte size
        size_t total = 0ull;

        // Pretty print contents
        for (const MappedEntry& entry: sortedEntries) {
            // Print padded name
            out << "\t'" << entry.name << "' ";
            Pad(out, entry.name.length(), kPadding);

            // Print size
            PostFix(out, entry.length);

            // Print allocation count
            out << " [" << entry.count << "]\n";

            // Count
            total += entry.length;
        }

        // Total counter
        out << "\n";
        out << "Total: ";
        PostFix(out, total);
        out << "\n";

        // Cleanup
        out.imbue(restore);
    }

    /// Count the total byte count
    /// \return number of bytes
    size_t CountTotal() {
        std::lock_guard guard(mutex);

        // Summarize
        size_t total = 0ull;
        for (auto&& it : taggedEntries) {
            total += it.second.length;
        }

        // OK
        return total;
    }
    
    Allocators GetAllocators() {
        return Allocators {
            .userData = this,
            .alloc = Allocate,
            .free = Free
        };
    }

private:
    static void* Allocate(void* self, size_t size, size_t align, AllocationTag tag) {
        return static_cast<TrackedAllocator*>(self)->Allocate(size, align, tag);
    }
    
    static void Free(void* self, void* data, size_t align) {
        static_cast<TrackedAllocator*>(self)->Free(data, align);
    }

private:
    /// Postfix a number
    /// \param out destination stream
    /// \param bytes byte count
    void PostFix(std::ostream& out, size_t bytes) {
        const auto length = static_cast<float>(bytes);
            
        if (length > 1e6f) {
            out << length / 1e6f << "mb";
        } else if (length > 1e3f) {
            out << length / 1e6f << "kb";
        } else {
            out << bytes << "b";
        }
    }

    /// Pad stream
    /// \param out destination stream
    /// \param length current length
    /// \param count expected length
    void Pad(std::ostream& out, size_t length, size_t count) {
        for (uint32_t i = 0; length < count && i < count - length; i++) {
            out << " ";
        }
    }
    
    size_t AlignedOffset(size_t align) {
        return align * ((HeaderSize + (align - 1)) / align);
    }

private:
    struct Header {
        size_t tag;
        size_t size;
    };

    static constexpr size_t HeaderSize = sizeof(Header);

private:
    struct MappedEntry {
        std::string_view name;
        int64_t length{0};
        int64_t count{0};
    };

    std::mutex mutex;

    std::unordered_map<uint64_t, MappedEntry> taggedEntries;
};
