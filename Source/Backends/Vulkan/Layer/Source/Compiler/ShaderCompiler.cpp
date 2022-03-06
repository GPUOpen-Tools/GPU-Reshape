#include <Backends/Vulkan/Compiler/ShaderCompiler.h>
#include <Backends/Vulkan/Compiler/ShaderCompilerDebug.h>
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

/// Enable serial compilation for debugging purposes
#ifndef SHADER_COMPILER_SERIAL
#   define SHADER_COMPILER_SERIAL 1
#endif

/// Enable instrumentation of a specific file for debugging purposes
///  ? Instrumentation of large applications can be difficult to debug and even harder to reproduce under the same conditions.
///    When such a fault occurs, it is very useful to simply be able to iterate on a spv binary file.
#ifndef SHADER_COMPILER_DEBUG_FILE
#   define SHADER_COMPILER_DEBUG_FILE 0
#endif

ShaderCompiler::ShaderCompiler(DeviceDispatchTable *table) : table(table) {

}

bool ShaderCompiler::Install() {
    dispatcher = registry->Get<Dispatcher>();
    if (!dispatcher) {
        return false;
    }

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
#if SHADER_COMPILER_SERIAL
    static std::mutex mutex;
    std::lock_guard guard(mutex);
#endif

    // Single file compile debugging
#if SHADER_COMPILER_DEBUG_FILE
    constexpr const char* kPath = "C:\\AMD\\GPUOpen-Tools\\gpu-validation\\Bin\\Debug\\Intermediate\\Debug\\UnrealEngine4.26\\GBV\\{EC9E76CD-F873-473E-A740-10F956D5C889}.source.spirv";

    // Stream in the debug binary
    std::ifstream stream(kPath, std::ios::in | std::ios::binary);
    std::vector<uint8_t> debugBinary((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    // Hack the code
    job.state->createInfoDeepCopy.createInfo.pCode = reinterpret_cast<const uint32_t*>(debugBinary.data());
    job.state->createInfoDeepCopy.createInfo.codeSize = debugBinary.size();
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
    spvJob.streamCount = exportCount;

    // Recompile the program
    if (!module->Recompile(
        job.state->createInfoDeepCopy.createInfo.pCode,
        static_cast<uint32_t>(job.state->createInfoDeepCopy.createInfo.codeSize / 4u),
        spvJob
    )) {
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
        return;
    }

    // Assign the instrument
    job.state->AddInstrument(job.instrumentationKey, instrument);

    // Destroy the module
    destroy(module, allocators);
}
