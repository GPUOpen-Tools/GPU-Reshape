#pragma once

// Layer
#include <Backends/DX12/InstrumentationInfo.h>
#include <Backends/DX12/States/ShaderStateKey.h>
#include <Backends/DX12/States/ShaderInstrumentationKey.h>
#include <Backends/DX12/Compiler/DXStream.h>

// Common
#include <Common/Containers/ReferenceObject.h>

// Std
#include <mutex>
#include <map>

// Forward declarations
struct DeviceState;
class DXModule;

struct ShaderState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~ShaderState();

    /// Add an instrument to this shader
    /// \param featureBitSet the enabled feature set
    /// \param byteCode the byteCode in question
    void AddInstrument(const ShaderInstrumentationKey& instrumentationKey, const DXStream& instrument);

    /// Get an instrument
    /// \param featureBitSet the enabled feature set
    /// \return nullptr if not found
    D3D12_SHADER_BYTECODE GetInstrument(const ShaderInstrumentationKey& instrumentationKey);

    /// Check if instrument is present
    /// \param featureBitSet the enabled feature set
    /// \return false if not found
    bool HasInstrument(const ShaderInstrumentationKey& instrumentationKey) {
        if (!instrumentationKey.featureBitSet) {
            return true; 
        }
        
        std::lock_guard lock(mutex);
        return instrumentObjects.count(instrumentationKey) > 0;
    }

    /// Reverse a future instrument
    /// \param instrumentationKey key to be used
    /// \return true if reserved
    bool Reserve(const ShaderInstrumentationKey& instrumentationKey);

    /// Originating key
    ///   ! Shader memory owned
    ShaderStateKey key;

    /// Backwards reference
    DeviceState* parent{nullptr};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;

    /// Instrumented objects lookup
    /// TODO: How do we manage lifetimes here?
    std::map<ShaderInstrumentationKey, DXStream> instrumentObjects;

    /// Parsing module
    ///   ! May not be indexed yet, indexing occurs during instrumentation.
    ///     Avoided during regular use to not tamper with performance.
    DXModule* module{nullptr};

    /// Unique ID
    uint64_t uid{0};

    /// byteCode specific lock
    std::mutex mutex;
};
