#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/DeviceDispatchTable.h>

// Backend
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>

// Common
#include <Common/Dispatcher.h>
#include <Common/Registry.h>

// Std
#include <fstream>

// Debugging
// TODO: Expand into a more versatile system
#define SPV_SHADER_COMPILER_DUMP 0

bool ShaderCompiler::Initialize() {
    dispatcher = registry->Get<Dispatcher>();
    if (!dispatcher) {
        return false;
    }

    // Get the feature host
    IFeatureHost *host = registry->Get<IFeatureHost>();
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

void ShaderCompiler::Add(DeviceDispatchTable* table, ShaderModuleState *state, uint64_t featureBitSet, DispatcherBucket *bucket) {
    auto data = new(registry->GetAllocators()) ShaderJob{
        .table = table,
        .state = state,
        .featureBitSet = featureBitSet
    };

    dispatcher->Add(BindDelegate(this, ShaderCompiler::Worker), data, bucket);
}

void ShaderCompiler::Worker(void *data) {
    auto *job = static_cast<ShaderJob *>(data);
    CompileShader(*job);
}

void ShaderCompiler::CompileShader(const ShaderJob &job) {
    // Create the module on demand
    if (!job.state->spirvModule) {
        job.state->spirvModule = new(registry->GetAllocators()) SpvModule(allocators);

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
    SpvModule* module = job.state->spirvModule->Copy();

    // Pass through all features
    for (IFeature *feature: features) {
        feature->Inject(*module->GetProgram());
    }

    // Recompile the program
    if (!module->Recompile(
        job.state->createInfoDeepCopy.createInfo.pCode,
        job.state->createInfoDeepCopy.createInfo.codeSize / 4u
    )) {
        return;
    }

#if SPV_SHADER_COMPILER_DUMP
    std::ofstream outBefore("module.before.spirv", std::ios_base::binary);
    outBefore.write(reinterpret_cast<const char*>(job.state->createInfoDeepCopy.createInfo.pCode), job.state->createInfoDeepCopy.createInfo.codeSize);
    outBefore.close();
#endif

    // Copy the deep creation info
    //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
    VkShaderModuleCreateInfo createInfo = job.state->createInfoDeepCopy.createInfo;
    createInfo.pCode = module->GetCode();
    createInfo.codeSize = module->GetSize();

    // Resulting module
    VkShaderModule instrument;

    // Attempt to compile the program
    VkResult result = job.table->next_vkCreateShaderModule(job.table->object, &createInfo, nullptr, &instrument);
    if (result != VK_SUCCESS) {
        return;
    }

#if SPV_SHADER_COMPILER_DUMP
    std::ofstream outAfter("module.after.spirv", std::ios_base::binary);
    outAfter.write(reinterpret_cast<const char*>(createInfo.pCode), createInfo.codeSize);
    outAfter.close();
#endif

    // Assign the instrument
    job.state->AddInstrument(job.featureBitSet, instrument);

    // Destroy the module
    destroy(module, allocators);
}
