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
    X##Set operator~(X##Set a) { return TBitSet<X>(a.value); } \
    X##Set operator|(X a, X b) { return TBitSet<X>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b)); } \
    X##Set operator|(X##Set a, X b) { return TBitSet<X>(a.value | static_cast<uint64_t>(b)); } \
    X##Set& operator|=(X##Set& a, X b) { a.value |= static_cast<uint64_t>(b); return a; } \
    X##Set& operator|=(X##Set& a, X##Set b) { a.value |= b.value; return a; } \
    X##Set& operator&=(X##Set& a, X##Set b) { a.value &= b.value; return a; } \
    bool operator&(X##Set a, X b) { return (static_cast<uint64_t>(a.value) & static_cast<uint64_t>(b)) != 0; } \
    bool operator&(X##Set a, X##Set b) { return (static_cast<uint64_t>(a.value) & static_cast<uint64_t>(b.value)) != 0; }
