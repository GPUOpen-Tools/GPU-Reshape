// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#include <Backends/DX12/Compiler/ShaderCompiler.h>
#include <Backends/DX12/Compiler/IDXModule.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>
#include <Backends/DX12/Compiler/ShaderCompilerDebug.h>
#include <Backends/DX12/Compiler/DXCompileJob.h>
#include <Backends/DX12/Compiler/DXParseJob.h>
#include <Backends/DX12/Compiler/DXStream.h>
#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>
#include <Backends/DX12/Compiler/DXBC/DXBCSigner.h>
#include <Backends/DX12/Compiler/DXBC/DXBCConverter.h>
#include <Backends/DX12/Compiler/Tags.h>
#include <Backends/DX12/ShaderData/ShaderDataHost.h>
#include <Backends/DX12/Compiler/Diagnostic/DiagnosticType.h>

// Backend
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/IShaderExportHost.h>
#include <Backend/Diagnostic/DiagnosticBucketScope.h>

// Common
#include <Common/Dispatcher/Dispatcher.h>
#include <Common/Registry.h>

ShaderCompiler::ShaderCompiler(DeviceState *device)
    : device(device),
      shaderFeatures(device->allocators.Tag(kAllocInstrumentation)),
      shaderData(device->allocators.Tag(kAllocInstrumentation)) {

}

bool ShaderCompiler::Install() {
    dispatcher = registry->Get<Dispatcher>();
    if (!dispatcher) {
        return false;
    }

    // Optional debug
    debug = registry->Get<ShaderCompilerDebug>();

    // Get all shader features
    for (const ComRef<IFeature>& feature : device->features) {
        auto shaderFeature = Cast<IShaderFeature>(feature);

        // Append null even if not found
        shaderFeatures.push_back(shaderFeature);
    }

    // Get the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    exportHost->Enumerate(&exportCount, nullptr);

    // Get the export host
    auto shaderDataHost = registry->Get<ShaderDataHost>();

    // Get number of resources
    uint32_t resourceCount;
    shaderDataHost->Enumerate(&resourceCount, nullptr, ShaderDataType::All);

    // Fill resources
    shaderData.resize(resourceCount);
    shaderDataHost->Enumerate(&resourceCount, shaderData.data(), ShaderDataType::All);

    // Get the signers
    dxilSigner = registry->Get<DXILSigner>();
    dxbcSigner = registry->Get<DXBCSigner>();

    // Get the converter
    dxbcConverter = registry->Get<DXBCConverter>();

    // OK
    return true;
}

void ShaderCompiler::Add(const ShaderJob& job, DispatcherBucket *bucket) {
    auto data = new(registry->GetAllocators(), kAllocInstrumentation) ShaderJob(job);
    job.diagnostic->totalJobs++;
    dispatcher->Add(BindDelegate(this, ShaderCompiler::Worker), data, bucket);
}

void ShaderCompiler::Worker(void *data) {
    auto *job = static_cast<ShaderJob *>(data);
    CompileShader(*job);
    destroy(job, allocators);
}

bool ShaderCompiler::InitializeModule(ShaderState *state) {
    // Instrumented pipelines are unique, however, originating modules may not be
    std::lock_guard moduleGuad(state->mutex);

    // Create the module on demand
    if (!state->module) {
        // Get type
        uint32_t type = *static_cast<const uint32_t *>(state->byteCode.pShaderBytecode);

        // Create the module
        switch (type) {
            default: {
                // Unknown type, just skip the job
                return false;
            }
            case 'CBXD': {
                state->module = new (allocators, kAllocModuleDXBC) DXBCModule(allocators.Tag(kAllocModuleDXBC), state->uid, GlobalUID::New());
                break;
            }
        }

        // Prepare job
        DXParseJob job;
        job.byteCode = state->byteCode.pShaderBytecode;
        job.byteLength = state->byteCode.BytecodeLength;
        job.pdbController = device->pdbController;
        job.dxbcConverter = dxbcConverter;

        // Try to parse the bytecode
        if (!state->module->Parse(job)) {
            return false;
        }
    }

    // OK
    return true;
}

void ShaderCompiler::CompileShader(const ShaderJob &job) {
#if SHADER_COMPILER_SERIAL
    static std::mutex mutex;
    std::lock_guard guard(mutex);
#endif

    // Diagnostic scope
    DiagnosticBucketScope scope(job.diagnostic->messages, job.state->uid);

    // Initialize module
    if (!InitializeModule(job.state)) {
        scope.Add(DiagnosticType::ShaderUnknownHeader, *static_cast<const uint32_t *>(job.state->byteCode.pShaderBytecode));
        ++job.diagnostic->failedJobs;
        return;
    }

    // Create a copy of the module, don't modify the source
    IDXModule *module = job.state->module->Copy();

    // Debugging
    std::filesystem::path debugPath;
    if (debug) {
        // Allocate path
        debugPath = debug->AllocatePath(module);

        // Add source module
        debug->Add(debugPath, "source", module);
    }

    // Get user map
    IL::ShaderDataMap& shaderDataMap = module->GetProgram()->GetShaderDataMap();

    // Add resources
    for (const ShaderDataInfo& info : shaderData) {
        shaderDataMap.Add(info);
    }

    // Pass through all features
    for (size_t i = 0; i < shaderFeatures.size(); i++) {
        if (!(job.instrumentationKey.featureBitSet & (1ull << i))) {
            continue;
        }

        // Inject marked shader feature
        shaderFeatures[i]->Inject(*module->GetProgram(), *job.dependentSpecialization);
    }

    // Instrumentation job
    DXCompileJob compileJob;
    compileJob.instrumentationKey = job.instrumentationKey;
    compileJob.streamCount = exportCount;
    compileJob.dxilSigner = dxilSigner;
    compileJob.dxbcSigner = dxbcSigner;
    compileJob.messages = scope;

    // Instrumented data
    DXStream stream(allocators);

    // Debugging
    if (!debugPath.empty()) {
        // Add instrumented module
        debug->Add(debugPath, "user", module);
    }

    // Attempt to recompile
    if (!module->Compile(compileJob, stream)) {
        ++job.diagnostic->failedJobs;
        return;
    }

    // Debugging
    if (!debugPath.empty()) {
        // Add instrumented module
        debug->Add(debugPath, "instrumented", module);
    }

    // Assign the instrument
    job.state->AddInstrument(job.instrumentationKey, stream);

    // Mark as passed
    ++job.diagnostic->passedJobs;

    // Destroy the module
    destroy(module, allocators);
}
