#pragma once

// Std
#include <cstdint>

/// Bit value
#define BIT(X) (1 << X)

template<typename T>
struct TBitSet {
    explicit TBitSet(uint64_t value) : value(value) {

    }

    TBitSet(T value) : value(static_cast<uint64_t>(value)) {

    }

    uint64_t value;
};

/// Declare a bit set
#define BIT_SET(X) \
    using X##Set = TBitSet<X>;\
    inline X##Set operator~(X##Set a) { return TBitSet<X>(a.value); } \
    inline X##Set operator|(X a, X b) { return TBitSet<X>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b)); } \
    inline X##Set operator|(X##Set a, X b) { return TBitSet<X>(a.value | static_cast<uint64_t>(b)); } \
    inline X##Set& operator|=(X##Set& a, X b) { a.value |= static_cast<uint64_t>(b); return a; } \
    inline X##Set& operator|=(X##Set& a, X##Set b) { a.value |= b.value; return a; } \
    inline X##Set& operator&=(X##Set& a, X##Set b) { a.value &= b.value; return a; } \
    inline bool operator&(X##Set a, X b) { return (static_cast<uint64_t>(a.value) & static_cast<uint64_t>(b)) != 0; } \
    inline bool operator&(X##Set a, X##Set b) { return (static_cast<uint64_t>(a.value) & static_cast<uint64_t>(b.value)) != 0; }
