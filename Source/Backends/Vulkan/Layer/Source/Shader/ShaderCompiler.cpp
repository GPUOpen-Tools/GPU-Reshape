#include <Backends/Vulkan/Shader/ShaderCompiler.h>
#include <Backends/Vulkan/Shader/SPIRVModule.h>

// Backend
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>

// Common
#include <Common/Dispatcher.h>
#include <Common/Registry.h>

bool ShaderCompiler::Initialize() {
    dispatcher = registry->Get<Dispatcher>();
    if (!dispatcher) {
        return false;
    }

    // Get the feature host
    IFeatureHost* host = registry->Get<IFeatureHost>();
    if (!host) {
        return false;
    }

    // Pool feature count
    uint32_t featureCount;
    host->Enumerate(&featureCount, nullptr);

    // Pool features
    features.resize(featureCount);
    host->Enumerate(&featureCount, features.data());

    // OK
    return true;
}

void ShaderCompiler::Add(ShaderModuleState *state, uint64_t featureBitSet, DispatcherBucket* bucket) {
    auto data = new (registry->GetAllocators()) ShaderJob {
        .state = state,
        .featureBitSet = featureBitSet
    };

    dispatcher->Add(BindDelegate(this, ShaderCompiler::Worker), data, bucket);
}

void ShaderCompiler::Worker(void* data) {
    auto* job = static_cast<ShaderJob*>(data);
    CompileShader(*job);
}

void ShaderCompiler::CompileShader(const ShaderJob& job) {
    // Create the module on demand
    if (!job.state->spirvModule) {
        job.state->spirvModule = new (registry->GetAllocators()) SPIRVModule();

        // Parse the module
        bool result = job.state->spirvModule->ParseModule(
            job.state->createInfoDeepCopy.createInfo.pCode,
            job.state->createInfoDeepCopy.createInfo.codeSize / 4u
        );

        // Failed?
        if (!result) {
            return;
        }
    }

    // Create a copy of the module, don't modify the source
    SPIRVModule module = *job.state->spirvModule;

    // Pass through all features
    for (IFeature* feature : features) {
        feature->Inject(module.GetProgram());
    }

    // Recompile the program
    if (!module.Recompile()) {
        return;
    }
}
