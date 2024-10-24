// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/States/ShaderModuleInstrumentationKey.h>
#include <Backends/Vulkan/Compiler/Diagnostic/PipelineCompilerDiagnostic.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <mutex>

class Dispatcher;
struct DispatcherBucket;
struct DeviceDispatchTable;

struct PipelineJob {
    PipelineState* state{ nullptr };

    /// TODO: Stack fallback
    uint64_t* pipelineLibraryInstrumentationKeys{nullptr};
    ShaderModuleInstrumentationKey* shaderModuleInstrumentationKeys{nullptr};

    /// Pipeline specific hash
    uint64_t combinedHash{ 0 };
};

class PipelineCompiler : public TComponent<PipelineCompiler> {
public:
    COMPONENT(PipelineCompiler);

    PipelineCompiler(DeviceDispatchTable* table);

    /// Install this compiler
    bool Install();

    /// Add a pipeline batch job
    /// \param states all pipeline states
    /// \param diagnostic pipeline diagnostic
    /// \param count the number of states
    /// \param bucket optional, dispatcher bucket
    void AddBatch(DeviceDispatchTable* table, PipelineCompilerDiagnostic* diagnostic, PipelineJob* jobs, uint32_t count, DispatcherBucket* bucket = nullptr);

protected:
    struct PipelineJobBatch {
        DeviceDispatchTable*        table;
        PipelineCompilerDiagnostic* diagnostic;
        PipelineJob*                jobs;
        uint32_t                    count;
    };

    /// Add a pipeline batch job
    /// \param table parent table
    /// \param diagnostic pipeline diagnostic
    /// \param jobs all pipeline jobs
    /// \param count the number of states
    /// \param bucket optional, dispatcher bucket
    void AddBatchOfType(DeviceDispatchTable* table, PipelineCompilerDiagnostic* diagnostic, const std::vector<PipelineJob>& jobs, PipelineType type, DispatcherBucket* bucket);

    /// Compile a given job
    void CompileGraphics(const PipelineJobBatch& job);
    void CompileCompute(const PipelineJobBatch& job);

    /// Worker entry
    void WorkerGraphics(void* userData);
    void WorkerCompute(void* userData);

    /// Set the shader module object
    /// \param createInfo the assigned creation info
    /// \param state the proxy state object
    /// \param key the instrumentation key to query
    /// \return success state
    bool SetShaderModuleObject(VkPipelineShaderStageCreateInfo &createInfo, ShaderModuleState *state, const ShaderModuleInstrumentationKey &key);

private:
    DeviceDispatchTable* table{nullptr};

    /// Shared lock
    std::mutex mutex;

    /// Job buckets
    std::vector<PipelineJob> graphicsJobs;
    std::vector<PipelineJob> computeJobs;

    /// Async dispatcher
    ComRef<Dispatcher> dispatcher{nullptr};
};
