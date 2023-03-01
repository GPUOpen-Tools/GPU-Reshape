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
        _default.name = "Untagged";
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
                it->second.name = tag.view;
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

        // Pretty print contents
        for (std::pair<const unsigned long long, MappedEntry> &it: taggedEntries) {
            out << "\t'" << it.second.name << "' length:" << it.second.length << " count:" << it.second.count << "\n";
        }

        // Cleanup
        out.imbue(restore);
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
