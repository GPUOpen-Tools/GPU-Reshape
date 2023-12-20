#pragma once

// Std
#include <cstdint>

// System
#include <intrin.h>

namespace Transform {
    /// Compute the hamming distance between two bit sets
    uint64_t HammingDistance(uint64_t a, uint64_t b) {
        uint64_t set = a ^ b;
        return __popcnt64(set);
    }
}
