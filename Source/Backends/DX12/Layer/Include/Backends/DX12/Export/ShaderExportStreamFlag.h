#pragma once

// Common
#include <Common/Enum.h>

enum class ShaderExportStreamFlag : uint32_t {
    /// All atomic counters are hosted on local memory
    ResidentCounters = BIT(0),

    /// All messaging data is hosted on local memory, will require a round trip to host memory
    ResidentExports = BIT(1),

    /// Perform resident export mirroring on an async transfer queue
    AsyncTransferQueue = BIT(2),

    /// Reject all overflowing messages, will increase shader complexity
    RejectOverflow = BIT(3),

    /// Use wave intrinsics to reduce atomic conflicts when sensible, will increase shader complexity
    WaveLocalExportOffsetAllocation = BIT(4),
};

BIT_SET(ShaderExportStreamFlag);
