#pragma once

// Layer
#include <Backends/DX12/InstrumentationInfo.h>
#include <Backends/DX12/ShaderInstrumentationKey.h>

// Common
#include <Common/Containers/ReferenceObject.h>

// Std
#include <mutex>
#include <map>

// Forward declarations
struct DeviceState;
struct DXModule;

struct ShaderState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~ShaderState() = default;

    /// Add an instrument to this shader
    /// \param featureBitSet the enabled feature set
    /// \param byteCode the byteCode in question
    void AddInstrument(const ShaderInstrumentationKey& key, D3D12_SHADER_BYTECODE instrument) {
        std::lock_guard lock(mutex);
        instrumentObjects[key] = instrument;
    }

    /// Get an instrument
    /// \param featureBitSet the enabled feature set
    /// \return nullptr if not found
    D3D12_SHADER_BYTECODE GetInstrument(const ShaderInstrumentationKey& key) {
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(key);
        if (it == instrumentObjects.end()) {
            return {};
        }

        return it->second;
    }

    /// Check if instrument is present
    /// \param featureBitSet the enabled feature set
    /// \return false if not found
    bool HasInstrument(const ShaderInstrumentationKey& key) {
        std::lock_guard lock(mutex);
        return instrumentObjects.count(key) > 0;
    }

    bool Reserve(const ShaderInstrumentationKey& key) {
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(key);
        if (it == instrumentObjects.end()) {
            instrumentObjects[key] = {};
            return true;
        }

        return false;
    }

    /// User byteCode
    D3D12_SHADER_BYTECODE byteCode;

    /// Backwards reference
    DeviceState* parent{nullptr};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;

    /// Instrumented objects lookup
    /// TODO: How do we manage lifetimes here?
    std::map<ShaderInstrumentationKey, D3D12_SHADER_BYTECODE> instrumentObjects;

    /// Parsing module
    ///   ! May not be indexed yet, indexing occurs during instrumentation.
    ///     Avoided during regular use to not tamper with performance.
    DXModule* module{nullptr};

    /// byteCode specific lock
    std::mutex mutex;
};
