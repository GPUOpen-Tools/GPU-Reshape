// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

// Std
#include <cstdint>

/// Bit value
#define BIT(X) (1 << X)

template<typename T>
struct TBitSet {
    TBitSet() : value(0) {

    }

    explicit TBitSet(uint64_t value) : value(value) {

    }

    TBitSet(T value) : value(static_cast<uint64_t>(value)) {

    }

    uint64_t value;
};

/// Declare a bit set
#define BIT_SET_NAMED(NAME, X) \
    static inline NAME operator~(NAME a) { return TBitSet<X>(~a.value); } \
    static inline NAME operator|(X a, X b) { return TBitSet<X>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b)); } \
    static inline NAME operator|(NAME a, X b) { return TBitSet<X>(a.value | static_cast<uint64_t>(b)); } \
    static inline NAME& operator|=(NAME& a, X b) { a.value |= static_cast<uint64_t>(b); return a; } \
    static inline NAME& operator|=(NAME& a, NAME b) { a.value |= b.value; return a; } \
    static inline NAME& operator&=(NAME& a, NAME b) { a.value &= b.value; return a; } \
    static inline bool operator&(NAME a, X b) { return (static_cast<uint64_t>(a.value) & static_cast<uint64_t>(b)) != 0; } \
    static inline bool operator&(NAME a, NAME b) { return (static_cast<uint64_t>(a.value) & static_cast<uint64_t>(b.value)) != 0; }

/// Declare a bit set
#define BIT_SET(X) \
    using X##Set = TBitSet<X>;\
    BIT_SET_NAMED(X##Set, X)
