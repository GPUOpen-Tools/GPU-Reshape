#pragma once

// Layer
#include <Backends/DX12/States/ShaderInstrumentationKey.h>

/// Job description
struct DXJob {
    /// The instrumentation key
    ShaderInstrumentationKey instrumentationKey{};

    /// The number of streams
    uint32_t streamCount{0};
};
