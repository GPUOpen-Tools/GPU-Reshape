// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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