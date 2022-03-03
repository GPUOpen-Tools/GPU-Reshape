#pragma once

// Std
#include <cstdint>

/// Common SPIRV header
struct SpvHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t generator;
    uint32_t bound;
    uint32_t reserved;
};
