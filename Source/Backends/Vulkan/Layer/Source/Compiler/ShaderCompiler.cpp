#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>

// Backend
#include <Backend/IFeatureHost.h>
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/IShaderExportHost.h>
#include <Backend/IL/PrettyPrint.h>

// Common
#include "Common/Dispatcher/Dispatcher.h"
#include <Common/Registry.h>

// Std
#include <fstream>

// Debugging
// TODO: Expand into a more versatile system
#define SPV_SHADER_COMPILER_DUMP 0

ShaderCompiler::ShaderCompiler(DeviceDispatchTable *table) : table(table) {

}

bool ShaderCompiler::Install() {
    dispatcher = registry->Get<Dispatcher>();
    if (!dispatcher) {
        return false;
    }

    // Get all shader features
    for (const ComRef<IFeature>& feature : table->features) {
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

void ShaderCompiler::Add(DeviceDispatchTable *table, ShaderModuleState *state, const ShaderModuleInstrumentationKey& instrumentationKey, DispatcherBucket *bucket) {
    auto data = new(registry->GetAllocators()) ShaderJob{
        .table = table,
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
#if SPV_SHADER_COMPILER_DUMP
    static std::mutex mutex;
    std::lock_guard guard(mutex);
#endif

    // Create the module on demand
    if (!job.state->spirvModule) {
        job.state->spirvModule = new(registry->GetAllocators()) SpvModule(allocators, job.state->uid);

        // Parse the module
        bool result = job.state->spirvModule->ParseModule(
            job.state->createInfoDeepCopy.createInfo.pCode,
            static_cast<uint32_t>(job.state->createInfoDeepCopy.createInfo.codeSize / 4u)
        );

        // Failed?
        if (!result) {
            return;
        }
    }

#if SPV_SHADER_COMPILER_DUMP
    std::ofstream outBeforeIL("module.before.txt");
    IL::PrettyPrint(*job.state->spirvModule->GetProgram(), outBeforeIL);
    outBeforeIL.close();
#endif

    // Create a copy of the module, don't modify the source
    SpvModule *module = job.state->spirvModule->Copy();

    // Pass through all features
    for (size_t i = 0; i < shaderFeatures.size(); i++) {
        if (!(job.instrumentationKey.featureBitSet & (1ull << i))) {
            continue;
        }

        // Inject marked shader feature
        shaderFeatures[i]->Inject(*module->GetProgram());
    }

#if SPV_SHADER_COMPILER_DUMP
    std::ofstream outAfterIL("module.after.txt");
    IL::PrettyPrint(*module->GetProgram(), outAfterIL);
    outAfterIL.close();
#endif

    // Spv job
    SpvJob spvJob;
    spvJob.instrumentationKey = job.instrumentationKey;
    spvJob.streamCount = exportCount;

    // Recompile the program
    if (!module->Recompile(
        job.state->createInfoDeepCopy.createInfo.pCode,
        static_cast<uint32_t>(job.state->createInfoDeepCopy.createInfo.codeSize / 4u),
        spvJob
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
    job.state->AddInstrument(job.instrumentationKey, instrument);

    // Destroy the module
    destroy(module, allocators);
}
