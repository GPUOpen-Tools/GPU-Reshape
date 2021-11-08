#pragma once

// Layer
#include "Vulkan.h"
#include "ReferenceObject.h"
#include "InstrumentationInfo.h"

// Deep Copy
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>

// Std
#include <atomic>

// Forward declarations
struct DeviceDispatchTable;
struct PipelineState;
class SPIRVModule;

struct ShaderModuleState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~ShaderModuleState();

    /// User module
    ///  ! May be nullptr if the top shader module has been destroyed
    VkShaderModule object{VK_NULL_HANDLE};

    /// Replaced shader module object, fx. instrumented version
    std::atomic<VkShaderModule> hotSwapObject{VK_NULL_HANDLE};

    /// Backwards reference
    DeviceDispatchTable* table;

    /// Recreation info
    VkShaderModuleCreateInfoDeepCopy createInfoDeepCopy;

    /// SPIRV module of the originating shader
    ///    ! On demand, may be nullptr
    SPIRVModule* spirvModule{nullptr};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
