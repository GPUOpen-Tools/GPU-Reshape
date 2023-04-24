#pragma once

// Layer
#include "PhysicalResourceSegment.h"

struct PhysicalResourceMappingTableSegment {
    /// Mapped identifier
    PhysicalResourceSegmentID id{0};

    /// Buffer offset
    uint32_t offset{0};

    /// Number of entries
    uint32_t length{0};
};
