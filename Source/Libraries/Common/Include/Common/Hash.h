#pragma once

// Std
#include <functional>

/// Combine a hash value
template <class T>
inline void CombineHash(std::size_t& hash, T value) {
    std::hash<T> hasher;
    hash ^= (hasher(value) + 0x9e3779b9 + (hash << 6) + (hash >> 2));
}
