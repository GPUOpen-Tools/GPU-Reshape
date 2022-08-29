#pragma once

// Std
#include <cstdint>

struct DXILBindingInfo {
    /// Target register space
    uint32_t space{};

    /// Underlying handle id
    uint32_t handleId{};

    /// Base register
    uint32_t _register{};

    /// Number of registers
    uint32_t count{};
};
