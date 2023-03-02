#include <Backends/DX12/Compiler/ShaderCompiler.h>
#include <Backends/DX12/Compiler/DXModule.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>
#include <Backends/DX12/Compiler/ShaderCompilerDebug.h>
#include <Backends/DX12/Compiler/DXJob.h>
#include <Backends/DX12/Compiler/DXStream.h>
#include <Backends/DX12/Compiler/DXIL/DXILSigner.h>
#include <Backends/DX12/Compiler/DXBC/DXBCSigner.h>
#include <Backends/DX12/Compiler/Tags.h>
#include <Backends/DX12/ShaderData/ShaderDataHost.h>

// Backend
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/IShaderExportHost.h>

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

    // OK
    return true;
}

void ShaderCompiler::Add(ShaderState *state, const ShaderInstrumentationKey& instrumentationKey, DispatcherBucket *bucket) {
    auto data = new(registry->GetAllocators(), kAllocInstrumentation) ShaderJob{
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

void ShaderCompiler::InitializeModule(ShaderState *state) {
    // Instrumented pipelines are unique, however, originating modules may not be
    std::lock_guard moduleGuad(state->mutex);

    // Create the module on demand
    if (!state->module) {
        // Get type
        uint32_t type = *static_cast<const uint32_t *>(state->key.byteCode.pShaderBytecode);

        // Create the module
        switch (type) {
            default: {
                // Unknown type, just skip the job
                return;
            }
            case 'CBXD': {
                state->module = new (allocators, kAllocModuleDXBC) DXBCModule(allocators.Tag(kAllocModuleDXBC), state->uid, GlobalUID::New());
                break;
            }
        }

        // Try to parse the bytecode
        if (!state->module->Parse(state->key.byteCode.pShaderBytecode, state->key.byteCode.BytecodeLength)) {
            return;
        }
    }
}

void ShaderCompiler::CompileShader(const ShaderJob &job) {
#if SHADER_COMPILER_SERIAL
    static std::mutex mutex;
    std::lock_guard guard(mutex);
#endif

    // Initialize module
    InitializeModule(job.state);

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
        shaderFeatures[i]->Inject(*module->GetProgram());
    }

    // Instrumentation job
    DXJob compileJob;
    compileJob.instrumentationKey = job.instrumentationKey;
    compileJob.streamCount = exportCount;
    compileJob.dxilSigner = dxilSigner;
    compileJob.dxbcSigner = dxbcSigner;

    // Instrumented data
    DXStream stream(allocators);

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
