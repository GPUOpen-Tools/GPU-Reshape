#pragma once

#include <Backends/Vulkan/States/ShaderModuleInstrumentationKey.h>
#include <Backends/Vulkan/States/PipelineLayoutBindingInfo.h>

/// Job description
struct SpvJob {
    /// The instrumentation key
    ShaderModuleInstrumentationKey instrumentationKey{};

    /// Layout information
    PipelineLayoutBindingInfo bindingInfo{};

    /// Sets up PRMT data for user descriptor mapping
    bool requiresUserDescriptorMapping{true};
};
