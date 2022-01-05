#pragma once

#include <cstdint>

namespace IL {
    /// Invalid, unmapped, source
    static constexpr uint32_t InvalidOffset = 0x7FFFFFFF;

    /// Source value, word offset for the source instruction
    struct Source {
        /// Create source from code
        static Source Code(uint32_t code) {
            Source source;
            source.codeOffset = code;
            source.modified = 0;
            return source;
        }

        /// Invalid source
        static Source Invalid() {
            Source source;
            source.codeOffset = InvalidOffset;
            source.modified = 0;
            return source;
        }

        /// Modify this source, only applies if the code offset is valid
        Source Modify() const {
            Source source;
            source.codeOffset = codeOffset;
            source.modified = IsValid();
            return source;
        }

        /// Is this source valid?
        bool IsValid() const {
            return codeOffset != InvalidOffset;
        }

        /// Can this source be trivially copied? (fx. memcpy)
        bool TriviallyCopyable() const {
            return codeOffset != InvalidOffset && !modified;
        }

        uint32_t codeOffset : 31;
        uint32_t modified   : 1;
    };


    /// Source span, represents a word region for the source instructions
    struct SourceSpan {
        SourceSpan AppendSpan() const {
            return {end, end};
        }

        uint32_t begin{InvalidOffset};
        uint32_t end{InvalidOffset};
    };

    /// Get the word count of a type
    template<typename T>
    inline uint32_t WordCount() {
        return sizeof(T) / sizeof(uint32_t);
    }
}