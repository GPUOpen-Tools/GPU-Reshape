#pragma once

// Std
#include <cstdint>

struct BreakpointStreamingHeader {
    /// The execution UID that owns this breakpoint
    uint32_t acquiredExecutionUID{0};

    /// The lock for checksum data
    uint32_t streamingChecksumLock{0};

    /// Expected checksum
    uint32_t streamingChecksum{0};
};

/// Number of dwords
static constexpr uint32_t BreakpointStreamingHeaderDWordCount = sizeof(BreakpointStreamingHeader) / sizeof(uint32_t);
