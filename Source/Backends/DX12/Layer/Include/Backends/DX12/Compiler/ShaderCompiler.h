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
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Compiler/ShaderCompilerDiagnostic.h>

// Backend
#include <Backend/ShaderData/ShaderDataInfo.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <mutex>

// Forward declarations
class Dispatcher;
struct DeviceState;
struct DispatcherBucket;
class IFeature;
class IShaderFeature;
class ShaderCompilerDebug;
class DXILSigner;
class DXBCSigner;
class DXBCConverter;

struct ShaderJob {
    /// State to compile
    ShaderState* state{ nullptr };

    /// Instrumentation key to apply
    ShaderInstrumentationKey instrumentationKey;

    /// Optional diagnostics
    ShaderCompilerDiagnostic* diagnostic{nullptr};

    /// Pipeline dependent specialization info
    MessageStream* dependentSpecialization{nullptr};
};

class ShaderCompiler : public TComponent<ShaderCompiler> {
public:
    COMPONENT(ShaderCompiler);

    ShaderCompiler(DeviceState* device);

    /// Install this compiler
    bool Install();

    /// Add a shader job
    /// \param job job to enqueue
    /// \param bucket optional, the dispatcher bucket
    void Add(const ShaderJob& job, DispatcherBucket *bucket = nullptr);

    /// Ensure a module is initialized
    /// \param state shader state
    bool InitializeModule(ShaderState* state);

protected:
    /// Compile a given job
    void CompileShader(const ShaderJob &job);

    /// Worker entry
    void Worker(void *userData);

private:
    DeviceState* device;

    /// Async dispatcher
    ComRef<Dispatcher> dispatcher;

    /// Debug device
    ComRef<ShaderCompilerDebug> debug;

    /// Signers
    ComRef<DXILSigner> dxilSigner;
    ComRef<DXBCSigner> dxbcSigner;

    /// Converters
    ComRef<DXBCConverter> dxbcConverter;

    /// All features
    Vector<ComRef<IShaderFeature>> shaderFeatures;

    /// All data
    Vector<ShaderDataInfo> shaderData;

    /// Number of exports
    uint32_t exportCount{0};
};
