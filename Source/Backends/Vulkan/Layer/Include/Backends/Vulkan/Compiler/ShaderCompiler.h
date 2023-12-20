// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/Compiler/Diagnostic/ShaderCompilerDiagnostic.h>

// Backend
#include <Backend/ShaderData/ShaderDataInfo.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <mutex>

class Dispatcher;
struct DispatcherBucket;
class IFeature;
class IShaderFeature;
class ShaderCompilerDebug;
class ShaderExportDescriptorAllocator;

struct ShaderJob {
    /// State to compile
    ShaderModuleState* state{ nullptr };

    /// Instrumentation key to apply
    ShaderModuleInstrumentationKey instrumentationKey;

    /// Optional diagnostics
    ShaderCompilerDiagnostic* diagnostic{nullptr};

    /// Pipeline dependent specialization stream
    MessageStream* dependentSpecialization{nullptr};
};

class ShaderCompiler : public TComponent<ShaderCompiler> {
public:
    COMPONENT(ShaderCompiler);

    ShaderCompiler(DeviceDispatchTable* table);

    /// Install this compiler
    bool Install();

    /// Add a shader job
    /// \param job job to enqueue
    /// \param bucket optional, the dispatcher bucket
    void Add(DeviceDispatchTable* table, const ShaderJob& job, DispatcherBucket *bucket = nullptr);

    /// Ensure a module is initialized
    /// \param state shader state
    bool InitializeModule(ShaderModuleState* state);

protected:
    struct ShaderJobEntry {
        DeviceDispatchTable *table;
        ShaderJob info;
    };

    /// Compile a given job
    void CompileShader(const ShaderJobEntry &job);

    /// Worker entry
    void Worker(void *userData);

private:
    DeviceDispatchTable* table{nullptr};

    /// Components
    ComRef<Dispatcher> dispatcher;
    ComRef<ShaderCompilerDebug> debug;
    ComRef<ShaderExportDescriptorAllocator> shaderExportDescriptorAllocator;

    /// All features
    std::vector<ComRef<IShaderFeature>> shaderFeatures;

    /// All data
    std::vector<ShaderDataInfo> shaderData;

    /// Number of exports
    uint32_t exportCount{0};
};
