#pragma once

/// Represents an inline message indirection
template<typename T>
struct MessagePtr {
    uint64_t thisOffset;

    T* Get() {
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t>(this) + thisOffset);
    }
};

/// Represents an inline message array
template<typename T>
struct MessageArray {
    uint64_t thisOffset;
    uint64_t count;

    T* Get() {
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + thisOffset);
    }

    T& operator[](size_t i) {
        return Get()[i];
    }
};