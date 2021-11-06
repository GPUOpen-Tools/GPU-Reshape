#pragma once

// Layer
#include "Vulkan.h"
#include "ReferenceObject.h"
#include "InstrumentationInfo.h"
#include "Shader/SPIRVModule.h"

// Deep Copy
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>

// Std
#include <atomic>

// Forward declarations
struct DeviceDispatchTable;

struct ShaderModuleState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~ShaderModuleState();

    /// Originating device
    ///  ! May be nullptr if the top shader module has been destroyed
    DeviceDispatchTable* table;

    /// User module
    VkShaderModule object{VK_NULL_HANDLE};

    /// Recreation info
    VkShaderModuleCreateInfoDeepCopy createInfoDeepCopy;

    /// SPIRV module of the originating shader
    SPIRVModule spirvModule;

    /// Replaced shader module object, fx. instrumented version
    std::atomic<VkShaderModule> hotSwapObject{VK_NULL_HANDLE};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;
};
