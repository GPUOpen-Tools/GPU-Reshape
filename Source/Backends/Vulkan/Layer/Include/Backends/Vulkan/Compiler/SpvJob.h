#pragma once

#include <Backends/Vulkan/States/ShaderModuleInstrumentationKey.h>

/// Job description
struct SpvJob {
    /// The instrumentation key
    ShaderModuleInstrumentationKey instrumentationKey{};

    /// The number of streams
    uint32_t streamCount{0};
};
