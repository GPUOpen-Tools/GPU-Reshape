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

struct ShaderBytecode {
    /// Owned bytecode
    const uint8_t* byteCode{nullptr};

    /// Byte size of bytecode
    size_t size{0};
};

struct ShaderState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~ShaderState();

    /// Add an instrument to this shader
    /// \param featureBitSet the enabled feature set
    /// \param byteCode the byteCode in question
    void AddInstrument(const ShaderInstrumentationKey& key, ShaderBytecode* instrument) {
        std::lock_guard lock(mutex);
        instrumentObjects[key] = instrument;
    }

    /// Get an instrument
    /// \param featureBitSet the enabled feature set
    /// \return nullptr if not found
    ShaderBytecode* GetInstrument(const ShaderInstrumentationKey& key) {
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
    bool HasInstrument(const ShaderInstrumentationKey& key) {
        std::lock_guard lock(mutex);
        return instrumentObjects.count(key) > 0;
    }

    bool Reserve(const ShaderInstrumentationKey& key) {
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(key);
        if (it == instrumentObjects.end()) {
            instrumentObjects[key] = nullptr;
            return true;
        }

        return false;
    }

    /// User byteCode
    ShaderBytecode byteCode;

    /// Backwards reference
    DeviceState* parent{nullptr};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;

    /// Instrumented objects lookup
    /// TODO: How do we manage lifetimes here?
    std::map<ShaderInstrumentationKey, ShaderBytecode*> instrumentObjects;

    /// byteCode specific lock
    std::mutex mutex;
};
