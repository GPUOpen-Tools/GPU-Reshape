// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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

// Common
#include <Common/String.h>

struct VDFParseContext {
public:
    /// Constructor
    VDFParseContext(const char* contents) : ptr(contents) {
        
    }

    /// Is this stream still good?
    /// \return true if the stream is good
    bool Good() {
        return *ptr != '\0';
    }

    /// Is the next character of type?
    /// \param ch character to test
    /// \return true if character matched
    bool Is(char ch) {
        SkipWhitespace();
        
        return *ptr == ch;
    }

    /// Consume if the next character is of type
    /// \param ch character to test
    /// \return true if character matched
    bool IsConsume(char ch) {
        SkipWhitespace();
        
        if (*ptr != ch) {
            return false;
        }

        *ptr++;
        return true;
    }

    /// Consume until a given character is met
    /// \param ch character to find
    /// \return view
    std::string_view ConsumeUntil(char ch) {
        SkipWhitespace();
        
        const char* begin = ptr;
        while (*ptr && *ptr != ch) {
            ptr++;
        }

        return std::string_view(begin, ptr);
    }

    /// Consume until a given character is met, skip the remaining
    /// \param ch character to find
    /// \return view
    std::string_view ConsumeWith(char ch) {
        std::string_view out = ConsumeUntil(ch);
        Skip();
        return out;
    }

    /// Skip a set of characters
    /// \param c number to skip
    void Skip(uint32_t c = 1) {
        for (uint32_t i = 0; i != c && *ptr; i++) {
            ptr++;
        }
    }

    /// Skip all whitespaces
    void SkipWhitespace() {
        while (*ptr && (std::iswhitespace(*ptr) || *ptr == '\n')) {
            ptr++;
        }
    }

private:
    /// Current contents pointer
    const char* ptr;
};
