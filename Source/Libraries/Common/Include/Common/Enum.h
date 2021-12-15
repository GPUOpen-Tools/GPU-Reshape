#pragma once

// Std
#include <cstdint>

/// Bit value
#define BIT(X) (1 << X)

/// Declare a bit set
#define BIT_SET(X) \
    using X##Set = uint64_t;\
    X##Set operator|(X a, X b) { return static_cast<uint64_t>(a) | static_cast<uint64_t>(b); } \
    X##Set operator|(X##Set a, X b) { return a | static_cast<uint64_t>(b); } \
    X##Set& operator|=(X##Set& a, X b) { return a |= static_cast<uint64_t>(b); } \
    bool operator&(X##Set a, X b) { return (static_cast<uint64_t>(a) & static_cast<uint64_t>(b)) != 0; }
