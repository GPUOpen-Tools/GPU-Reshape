#pragma once

// Common
#include "CRC.h"

struct ShortHashString {
    constexpr ShortHashString(const char* name) : name(name) {
        hash = StringCRC32Short(name);
    }

    /// Original string
    const char* name;

    /// Computed hash
    uint32_t hash;
};
