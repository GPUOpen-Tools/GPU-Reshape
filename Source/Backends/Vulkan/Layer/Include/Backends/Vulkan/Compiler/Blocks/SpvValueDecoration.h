#pragma once

// Std
#include <cstdint>
#include <vector>

struct SpvValueDecoration {
    /// Bound descriptor set
    uint32_t descriptorSet{UINT32_MAX};

    /// Offset within the descriptor set
    uint32_t descriptorOffset{UINT32_MAX};

    /// Offset within a block
    uint32_t blockOffset{UINT32_MAX};

    /// All member decorations
    std::vector<SpvValueDecoration> memberDecorations;
};
