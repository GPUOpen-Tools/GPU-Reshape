#pragma once

// Layer
#include "Vulkan.h"
#include "ReferenceObject.h"
#include "InstrumentationInfo.h"

// Deep Copy
#include <Backends/Vulkan/DeepCopyObjects.Gen.h>

// Std
#include <mutex>
#include <atomic>
#include <map>

// Forward declarations
struct DeviceDispatchTable;
struct PipelineState;
class SpvModule;

struct ShaderModuleState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~ShaderModuleState();

    /// Add an instrument to this module
    /// \param featureBitSet the enabled feature set
    /// \param module the module in question
    void AddInstrument(uint64_t featureBitSet, VkShaderModule module) {
        std::lock_guard lock(mutex);
        instrumentObjects[featureBitSet] = module;
    }

    /// Get an instrument
    /// \param featureBitSet the enabled feature set
    /// \return nullptr if not found
    VkShaderModule GetInstrument(uint64_t featureBitSet) {
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(featureBitSet);
        if (it == instrumentObjects.end()) {
            return nullptr;
        }

        return it->second;
    }

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
    SpvModule* spirvModule{nullptr};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;

    /// Instrumented objects lookup
    /// TODO: How do we manage lifetimes here?
    std::map<uint64_t, VkShaderModule> instrumentObjects;

    /// Module specific lock
    std::mutex mutex;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
