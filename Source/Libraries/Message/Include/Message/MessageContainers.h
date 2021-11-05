#pragma once

// Common
#include <Common/Align.h>

// Std
#include <cstring>

/// Represents an inline message indirection
template<typename T>
struct ALIGN_TO(4) MessagePtr {
    uint64_t thisOffset;

    T* Get() {
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t>(this) + thisOffset);
    }
};

static_assert(sizeof(MessagePtr<void>) == 8, "Malformed message pointer size");

/// Represents an inline message array
template<typename T>
struct ALIGN_TO(4) MessageArray {
    uint64_t thisOffset;
    uint64_t count;

    [[nodiscard]]
    T* Get() {
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + thisOffset);
    }

    [[nodiscard]]
    const T* Get() const {
        return reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(this) + thisOffset);
    }

    T& operator[](size_t i) {
        return Get()[i];
    }

    const T& operator[](size_t i) const {
        return Get()[i];
    }
};

static_assert(sizeof(MessageArray<uint32_t>) == 16, "Malformed message array size");

struct ALIGN_TO(4) MessageString {
    MessageString& operator=(const char* str) {
        std::memcpy(data.Get(), str, data.count * sizeof(char));
        return *this;
    }

    [[nodiscard]]
    std::string_view View() const {
        return std::string_view(data.Get(), data.count);
    }

    MessageArray<char> data;
};

static_assert(sizeof(MessageString) == 16, "Malformed message string size");