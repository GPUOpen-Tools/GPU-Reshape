#include <Backends/DX12/Compiler/ShaderCompiler.h>
#include <Backends/DX12/Compiler/DXModule.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>

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

    // Get all shader features
    for (const ComRef<IFeature>& feature : device->features) {
        auto shaderFeature = Cast<IShaderFeature>(feature);

        // Append null even if not found
        shaderFeatures.push_back(shaderFeature);
    }

    // Get the export host
    auto exportHost = registry->Get<IShaderExportHost>();
    exportHost->Enumerate(&exportCount, nullptr);

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
    // Create the module on demand
    if (!job.state->module) {
        // Get type
        uint32_t type = *static_cast<const uint64_t *>(job.state->key.byteCode.pShaderBytecode);

        // Create the module
        switch (type) {
            default: {
                // Unknown type, just skip the job
                return;
            }
            case 'CBXD': {
                job.state->module = new (allocators) DXBCModule(allocators);
            }
        }

        // Try to parse the bytecode
        if (!job.state->module->Parse(job.state->key.byteCode.pShaderBytecode, job.state->key.byteCode.BytecodeLength)) {
            return;
        }
    }

    // Create a copy of the module, don't modify the source
    DXModule *module = job.state->module->Copy();

    // Pass through all features
    for (size_t i = 0; i < shaderFeatures.size(); i++) {
        if (!(job.instrumentationKey.featureBitSet & (1ull << i))) {
            continue;
        }

        // Inject marked shader feature
        shaderFeatures[i]->Inject(*module->GetProgram());
    }

    // Passthrough for now
    D3D12_SHADER_BYTECODE instrumentedByteCode = job.state->key.byteCode;

    // Assign the instrument
    job.state->AddInstrument(job.instrumentationKey, instrumentedByteCode);

    // Destroy the module
    destroy(module, allocators);
}
