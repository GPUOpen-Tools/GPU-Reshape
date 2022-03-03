#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/InstrumentationInfo.h>
#include <Backends/Vulkan/States/ShaderModuleInstrumentationKey.h>

// Common
#include <Common/Containers/ReferenceObject.h>

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
    void AddInstrument(const ShaderModuleInstrumentationKey& key, VkShaderModule module) {
        std::lock_guard lock(mutex);
        instrumentObjects[key] = module;
    }

    /// Get an instrument
    /// \param featureBitSet the enabled feature set
    /// \return nullptr if not found
    VkShaderModule GetInstrument(const ShaderModuleInstrumentationKey& key) {
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(key);
        if (it == instrumentObjects.end()) {
            return nullptr;
        }

        return it->second;
    }

    /// Check if instrument is present
    /// \param featureBitSet the enabled feature set
    /// \return false if not found
    bool HasInstrument(const ShaderModuleInstrumentationKey& key) {
        std::lock_guard lock(mutex);
        return instrumentObjects.count(key) > 0;
    }

    bool Reserve(const ShaderModuleInstrumentationKey& key) {
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(key);
        if (it == instrumentObjects.end()) {
            instrumentObjects[key] = nullptr;
            return true;
        }

        return false;
    }

    /// User module
    ///  ! May be nullptr if the top shader module has been destroyed
    VkShaderModule object{VK_NULL_HANDLE};

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
    std::map<ShaderModuleInstrumentationKey, VkShaderModule> instrumentObjects;

    /// Module specific lock
    std::mutex mutex;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
