#include <Backends/DX12/Compiler/ShaderCompiler.h>
#include <Backends/DX12/Compiler/DXModule.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>
#include <Backends/DX12/Compiler/ShaderCompilerDebug.h>
#include <Backends/DX12/Compiler/DXJob.h>
#include <Backends/DX12/Compiler/DXStream.h>
#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>
#include <Backends/DX12/Compiler/DXBC/DXBCSigner.h>

// Backend
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/IShaderExportHost.h>

// Common
#include <Common/Dispatcher/Dispatcher.h>
#include <Common/Registry.h>

ShaderCompiler::ShaderCompiler(DeviceState* device) : device(device) {

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

    // Get the dxil signer
    dxilSigner = registry->Get<DXILSigner>();
    dxbcSigner = registry->Get<DXBCSigner>();

    // OK
    return true;
}

void ShaderCompiler::Add(ShaderState *state, const ShaderInstrumentationKey& instrumentationKey, DispatcherBucket *bucket) {
    auto data = new(registry->GetAllocators()) ShaderJob{
        .state = state,
        .instrumentationKey = instrumentationKey
    };

    dispatcher->Add(BindDelegate(this, ShaderCompiler::Worker), data, bucket);
}

void ShaderCompiler::Worker(void *data) {
    auto *job = static_cast<ShaderJob *>(data);
    CompileShader(*job);
    destroy(job, allocators);
}

void ShaderCompiler::CompileShader(const ShaderJob &job) {
#if SHADER_COMPILER_SERIAL
    static std::mutex mutex;
    std::lock_guard guard(mutex);
#endif

    // Create the module on demand
    if (!job.state->module) {
        // Instrumented pipelines are unique, however, originating modules may not be
        std::lock_guard moduleGuad(job.state->mutex);

        // Get type
        uint32_t type = *static_cast<const uint64_t *>(job.state->key.byteCode.pShaderBytecode);

        // Create the module
        switch (type) {
            default: {
                // Unknown type, just skip the job
                return;
            }
            case 'CBXD': {
                job.state->module = new (allocators) DXBCModule(allocators, job.state->uid, GlobalUID::New());
                break;
            }
        }

        // Try to parse the bytecode
        if (!job.state->module->Parse(job.state->key.byteCode.pShaderBytecode, job.state->key.byteCode.BytecodeLength)) {
            return;
        }
    }

    // Create a copy of the module, don't modify the source
    DXModule *module = job.state->module->Copy();

    // Debugging
    std::filesystem::path debugPath;
    if (debug) {
        // Allocate path
        debugPath = debug->AllocatePath(module);

        // Add source module
        debug->Add(debugPath, "source", module);
    }

    // Pass through all features
    for (size_t i = 0; i < shaderFeatures.size(); i++) {
        if (!(job.instrumentationKey.featureBitSet & (1ull << i))) {
            continue;
        }

        // Inject marked shader feature
        shaderFeatures[i]->Inject(*module->GetProgram());
    }

    // Instrumentation job
    DXJob compileJob;
    compileJob.instrumentationKey = job.instrumentationKey;
    compileJob.streamCount = exportCount;
    compileJob.dxilSigner = dxilSigner;
    compileJob.dxbcSigner = dxbcSigner;

    // Instrumented data
    DXStream stream;

    // Debugging
    if (!debugPath.empty()) {
        // Add instrumented module
        debug->Add(debugPath, "user", module);
    }

    // Attempt to recompile
    if (!module->Compile(compileJob, stream)) {
        return;
    }

    // Debugging
    if (!debugPath.empty()) {
        // Add instrumented module
        debug->Add(debugPath, "instrumented", module);
    }

    // Assign the instrument
    job.state->AddInstrument(job.instrumentationKey, stream);

    // Destroy the module
    destroy(module, allocators);
}
