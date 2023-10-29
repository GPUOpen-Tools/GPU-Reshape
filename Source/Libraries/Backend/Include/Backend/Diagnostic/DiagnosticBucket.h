#pragma once

// Backend
#include "DiagnosticMessage.h"

// Common
#include <Common/Containers/LinearBlockAllocator.h>

// Std
#include <vector>
#include <mutex>

template<typename T>
class DiagnosticBucket {
    using BucketType = std::vector<DiagnosticMessage<T>>;
    
public:
    /// Default constructor
    DiagnosticBucket() = default;

    /// Copy constructor, do not inherit the allocator
    DiagnosticBucket(const DiagnosticBucket& other) : messages(other.messages) {
        
    }

    /// Add a new message
    /// \param type message type
    /// \param args message arguments
    template<typename... TX>
    void Add(T type, TX&&... args) {
        std::lock_guard guard(mutex);

        // Byte count for arguments
        uint32_t argumentSize{0};

        // Argument stack base address
        uint8_t* argumentBase{nullptr};

        // Any argument data?
        if constexpr (sizeof...(TX) > 0) {
            // Determine size
            argumentSize = WriteArguments(nullptr, args...);

            // Allocate and write arguments
            argumentBase = allocator.AllocateArray<uint8_t>(argumentSize);
            WriteArguments(argumentBase, args...);
        }

        // Add message
        DiagnosticMessage<T>& message = messages.emplace_back();
        message.type = type;
        message.argumentBase = argumentBase;
        message.argumentSize = argumentSize;
    }

    /// Get starting iterator
    BucketType::const_iterator begin() const {
        return messages.begin();
    }

    /// Get ending iterator
    BucketType::const_iterator end() const {
        return messages.end();
    }

private:
    /// Argument writer, empty stub
    uint32_t WriteArguments(uint8_t* base) {
        return 0;
    }
    
    /// Argument writer
    template<typename T, typename... TX>
    uint32_t WriteArguments(uint8_t* base, T&& first, TX&&... args) {
        // Writing pass?
        if (base) {
            std::memcpy(base, &first, sizeof(first));
            base += sizeof(T);
        }

        // Accumulate size
        return sizeof(T) + WriteArguments(base, args...);
    }

private:
    /// All messages
    BucketType messages;

    /// Internal allocator
    LinearBlockAllocator<1024> allocator;

    /// Diagnostic buckets may be threaded
    std::mutex mutex;
};
