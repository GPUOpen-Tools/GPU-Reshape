#pragma once

// Std
#include <cstdint>

struct BridgeInfo {
    /// Total number of bytes written
    uint64_t bytesWritten{0};

    /// Total number of bytes read
    uint64_t bytesRead{0};
};
