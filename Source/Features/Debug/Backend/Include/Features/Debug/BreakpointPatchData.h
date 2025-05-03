#pragma once

// Std
#include <cstdint>

struct BreakpointPatchData {
    /// The allocation offset
    uint32_t allocationDWordOffset{0};

    /// The total number of offsets for the stream
    uint32_t streamDWordCount{0};
};
