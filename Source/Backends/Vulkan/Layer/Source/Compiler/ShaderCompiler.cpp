#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Compiler/ShaderCompilerDebug.h>
#include <Backends/Vulkan/Compiler/SpvModule.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/Vulkan/ShaderData/ShaderDataHost.h>

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

ShaderCompiler::ShaderCompiler(DeviceDispatchTable *table) : table(table) {

}

bool ShaderCompiler::Install() {
    dispatcher = registry->Get<Dispatcher>();
    if (!dispatcher) {
        return false;
    }

    shaderExportDescriptorAllocator = registry->Get<ShaderExportDescriptorAllocator>();

    // Optional debug
    debug = registry->Get<ShaderCompilerDebug>();

    // Get all shader features
    for (const ComRef<IFeature>& feature : table->features) {
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

    // OK
    return true;
}

void ShaderCompiler::Add(DeviceDispatchTable *table, ShaderModuleState *state, ShaderCompilerDiagnostic* diagnostic, const ShaderModuleInstrumentationKey& instrumentationKey, DispatcherBucket *bucket) {
    auto data = new(registry->GetAllocators()) ShaderJob{
        .table = table,
        .state = state,
        .diagnostic = diagnostic,
        .instrumentationKey = instrumentationKey
    };

    dispatcher->Add(BindDelegate(this, ShaderCompiler::Worker), data, bucket);
}

bool ShaderCompiler::InitializeModule(ShaderModuleState *state) {
    // Create the module on demand
    if (!state->spirvModule) {
        state->spirvModule = new(registry->GetAllocators()) SpvModule(allocators, state->uid);

        // Parse the module
        bool result = state->spirvModule->ParseModule(
            state->createInfoDeepCopy.createInfo.pCode,
            static_cast<uint32_t>(state->createInfoDeepCopy.createInfo.codeSize / 4u)
        );

        // Failed?
        if (!result) {
            return false;
        }
    }

    // OK
    return true;
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

    // Single file compile debugging
#if SHADER_COMPILER_DEBUG_FILE
    constexpr const char* kPath = "C:\\AMD\\GPUOpen-Tools\\gpu-validation\\Bin\\Clang\\Debug\\Intermediate\\Debug\\Cauldron v1.4\\SampleVK v1.4.1\\Vulkan\\{0C9E10E4-945A-4C72-AC8E-E436B28EEB75}.source.spirv";

    // Stream in the debug binary
    std::ifstream stream(kPath, std::ios::in | std::ios::binary);
    std::vector<uint8_t> debugBinary((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    // Hack the code
    job.state->createInfoDeepCopy.createInfo.pCode = reinterpret_cast<const uint32_t*>(debugBinary.data());
    job.state->createInfoDeepCopy.createInfo.codeSize = debugBinary.size();
#endif

    // Ensure state is initialized
    if (!InitializeModule(job.state)) {
        ++job.diagnostic->failedJobs;
        return;
    }

    // Passed initial check?
    bool validSource{true};

    // Debug
    std::filesystem::path debugPath;
    if (debug) {
        // Allocate path
        debugPath = debug->AllocatePath(job.state->spirvModule);

        // Dump source
        debug->Add(debugPath, "source", job.state->spirvModule, job.state->createInfoDeepCopy.createInfo.pCode, job.state->createInfoDeepCopy.createInfo.codeSize);

        // Validate the source module
        validSource = debug->Validate(job.state->createInfoDeepCopy.createInfo.pCode, job.state->createInfoDeepCopy.createInfo.codeSize / sizeof(uint32_t));
    }

    // Create a copy of the module, don't modify the source
    SpvModule *module = job.state->spirvModule->Copy();

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

    // Spv job
    SpvJob spvJob;
    spvJob.instrumentationKey = job.instrumentationKey;
    spvJob.bindingInfo = shaderExportDescriptorAllocator->GetBindingInfo();

    // Recompile the program
    if (!module->Recompile(
        job.state->createInfoDeepCopy.createInfo.pCode,
        static_cast<uint32_t>(job.state->createInfoDeepCopy.createInfo.codeSize / 4u),
        spvJob
    )) {
        ++job.diagnostic->failedJobs;
        return;
    }

    // Copy the deep creation info
    //  This is safe, as the change is done on the copy itself, the original deep copy is untouched
    VkShaderModuleCreateInfo createInfo = job.state->createInfoDeepCopy.createInfo;
    createInfo.pCode = module->GetCode();
    createInfo.codeSize = module->GetSize();

    // Naive validation
    ASSERT(*createInfo.pCode == SpvMagicNumber, "Invalid SPIR-V magic number");

    // Debug
    if (!debugPath.empty()) {
        // Dump instrumented
        debug->Add(debugPath, "instrumented", module, module->GetCode(), module->GetSize());

        // Try to validate the instrumented module
        if (validSource) {
            ENSURE(debug->Validate(module->GetCode(), module->GetSize() / sizeof(uint32_t)), "Instrumentation produced incorrect binaries");
        }
    }

    // Resulting module
    VkShaderModule instrument;

    // Attempt to compile the program
    VkResult result = job.table->next_vkCreateShaderModule(job.table->object, &createInfo, nullptr, &instrument);
    if (result != VK_SUCCESS) {
        ++job.diagnostic->failedJobs;
        return;
    }

    // Assign the instrument
    job.state->AddInstrument(job.instrumentationKey, instrument);

    // Destroy the module
    destroy(module, allocators);
}
