// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
class IDXModule;

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
    ShaderStateKey key;

    /// Byte code copy
    D3D12_SHADER_BYTECODE byteCode;

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
    IDXModule* module{nullptr};

    /// Unique ID
    uint64_t uid{0};

    /// byteCode specific lock
    std::mutex mutex;
};
