#pragma once

// Layer
#include <cstdint>

struct VersionSegmentationPoint {
    /// Version head
    uint32_t id{UINT32_MAX};

    /// Is this version segmented?
    bool segmented{false};
};
