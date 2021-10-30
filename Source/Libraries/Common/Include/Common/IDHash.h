#pragma once

// Std
#include <cstdint>

/// Simple 32bit name hash
uint32_t IDHash(const char *str) {
    uint32_t hash = 0;
    for (auto p = reinterpret_cast<const uint8_t*>(str); *p != '\0'; p++) {
        hash = 37 * hash + *p;
    }

    return hash;
}
