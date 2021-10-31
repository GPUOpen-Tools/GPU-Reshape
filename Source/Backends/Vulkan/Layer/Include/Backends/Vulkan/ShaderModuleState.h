#pragma once

// Layer
#include "Vulkan.h"
#include "ReferenceObject.h"
#include "InstrumentationInfo.h"

// Std
#include <atomic>

struct ShaderModuleState : public ReferenceObject {
    /// User module
    VkShaderModule object{nullptr};

    /// Replaced shader module object, fx. instrumented version
    std::atomic<VkShaderModule> hotSwapObject{nullptr};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;
};
