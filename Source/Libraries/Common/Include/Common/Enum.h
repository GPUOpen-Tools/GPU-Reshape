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
