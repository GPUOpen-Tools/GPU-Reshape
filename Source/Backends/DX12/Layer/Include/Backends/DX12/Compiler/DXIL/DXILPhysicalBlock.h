#pragma once

// Std
#include <cstdint>

struct DXILPhysicalBlock {
    /// Block starting address
    const uint8_t* ptr{nullptr};

    /// Size of this block
    uint32_t length{0};
};
