#pragma once

// Layer
#include "Vulkan.h"
#include "ReferenceObject.h"
#include "InstrumentationInfo.h"

// Deep Copy
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>

// Std
#include <atomic>

struct ShaderModuleState : public ReferenceObject {
    /// User module
    VkShaderModule object{VK_NULL_HANDLE};

    /// Recreation info
    VkShaderModuleCreateInfoDeepCopy createInfoDeepCopy;

    /// Replaced shader module object, fx. instrumented version
    std::atomic<VkShaderModule> hotSwapObject{VK_NULL_HANDLE};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;
};
