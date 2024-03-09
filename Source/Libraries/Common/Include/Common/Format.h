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

// Format
#define FMT_CONSTEVAL
#include <fmt/core.h>

/// Format a message
///   ? See https://github.com/fmtlib/fmt/tree/8.1.1 for formatting documentation
/// \param message message contents
/// \param args arguments to message contents
template<typename... T>
inline std::string Format(const char* message, T&&... args) {
    return fmt::format(message, std::forward<T>(args)...);
}

/// Format a message to a fixed length buffer
///   ? See https://github.com/fmtlib/fmt/tree/8.1.1 for formatting documentation
/// \param buffer the destination buffer
/// \param message message contents
/// \param args arguments to message contents
/// \return written length
template <size_t SIZE, typename... T>
inline uint64_t FormatArray(char (&buffer)[SIZE], const char* message, T&&... args) {
    return fmt::format_to_n(buffer, SIZE, message, std::forward<T>(args)...).size;
}

/// Format a message to a fixed length buffer with null termination
///   ? See https://github.com/fmtlib/fmt/tree/8.1.1 for formatting documentation
/// \param buffer the destination buffer
/// \param message message contents
/// \param args arguments to message contents
template <size_t SIZE, typename... T>
inline void FormatArrayTerminated(char (&buffer)[SIZE], const char* message, T&&... args) {
    uint64_t length = FormatArray(buffer, message, std::forward<T>(args)...);
    if (length >= SIZE) {
        return;
    }

    buffer[length] = '\0';
}
