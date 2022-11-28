#pragma once

// Std
#include <cstdint>

struct SpvValueDecoration {
    /// Bound descriptor set
    uint32_t descriptorSet{UINT32_MAX};

    /// Offset within the descriptor set
    uint32_t offset{UINT32_MAX};
};
