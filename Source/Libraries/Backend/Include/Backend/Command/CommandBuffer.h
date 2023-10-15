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

    /// Add new comamnd
    /// \param command command data, must be top type
    void Append(const void* commandData, size_t length) {
        const auto* dataU8 = static_cast<const uint8_t*>(commandData);
        data.insert(data.end(), dataU8, dataU8 + length);
    }

    /// Increment the number of commands
    void Increment() {
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
