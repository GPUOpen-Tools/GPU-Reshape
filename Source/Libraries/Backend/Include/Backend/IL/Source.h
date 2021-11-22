#pragma once

#include <cstdint>

namespace IL {
    /// Source value, word offset for the source instruction
    using Source = uint32_t;

    /// Invalid, unmapped, source
    static constexpr Source InvalidSource = 0xFFFFFFFF;

    /// Source span, represents a word region for the source instructions
    struct SourceSpan {
        Source begin;
        Source end;
    };

    /// Get the word count of a type
    template<typename T>
    inline uint32_t WordCount() {
        return sizeof(T) / sizeof(uint32_t);
    }
}