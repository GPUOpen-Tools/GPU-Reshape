#pragma once

// Backend
#include "Command.h"

// Std
#include <vector>

struct CommandBuffer {
    struct ConstIterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Command;
        using pointer = value_type *;
        using reference = value_type &;

        /// Comparison
        bool operator!=(const ConstIterator &other) const {
            return ptr != other.ptr;
        }

        /// Get the command
        const Command &Get() const {
            return *reinterpret_cast<const Command *>(ptr);
        }

        /// Dereference
        const Command &operator*() const {
            return Get();
        }

        /// Accessor
        const Command *operator->() const {
            return &Get();
        }

        /// Implicit
        operator const Command &() const {
            return Get();
        }

        /// Post increment
        ConstIterator operator++(int) {
            ConstIterator self = *this;
            ++(*this);
            return self;
        }

        /// Pre increment
        ConstIterator &operator++() {
            ptr = ptr + Get().commandSize;
            return *this;
        }

        /// Is this iterator valid?
        bool IsValid() const {
            return ptr != nullptr;
        }

        /// Current offset
        const uint8_t *ptr{nullptr};
    };

    /// Add new comamnd
    /// \param command command data, must be top type
    template<typename T>
    void Add(const T &command) {
        auto *commandData = reinterpret_cast<const uint8_t *>(&command);
        data.insert(data.end(), commandData, commandData + command.commandSize);
        count++;
    }

    /// Clear all command data
    void Clear() {
        data.clear();
        count = 0;
    }

    /// Get number of commands
    uint32_t Count() const {
        return count;
    }

    /// Get immutable beginning
    ConstIterator begin() const {
        return ConstIterator{data.data()};
    }

    /// Get immutable ending
    ConstIterator end() const {
        return ConstIterator{data.data() + data.size()};
    }

private:
    /// Number of messages
    uint32_t count{0};

    /// Contained data
    std::vector<uint8_t> data;
};
