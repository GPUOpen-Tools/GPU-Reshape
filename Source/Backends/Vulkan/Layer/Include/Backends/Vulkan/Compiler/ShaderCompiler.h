#pragma once

// Layer
#include <Backends/Vulkan/States/ShaderModuleState.h>
#include <Backends/Vulkan/Compiler/ShaderCompilerDiagnostic.h>

// Backend
#include <Backend/ShaderData/ShaderDataInfo.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <mutex>

class Dispatcher;
struct DispatcherBucket;
class IFeature;
class IShaderFeature;
class ShaderCompilerDebug;
class ShaderExportDescriptorAllocator;

struct ShaderJob {
    /// State to compile
    ShaderModuleState* state{ nullptr };

    /// Instrumentation key to apply
    ShaderModuleInstrumentationKey instrumentationKey;

    /// Optional diagnostics
    ShaderCompilerDiagnostic* diagnostic{nullptr};

    /// Pipeline dependent specialization stream
    MessageStream* dependentSpecialization{nullptr};
};

class ShaderCompiler : public TComponent<ShaderCompiler> {
public:
    COMPONENT(ShaderCompiler);

    ShaderCompiler(DeviceDispatchTable* table);

    /// Install this compiler
    bool Install();

    /// Add a shader job
    /// \param job job to enqueue
    /// \param bucket optional, the dispatcher bucket
    void Add(DeviceDispatchTable* table, const ShaderJob& job, DispatcherBucket *bucket = nullptr);

    /// Ensure a module is initialized
    /// \param state shader state
    bool InitializeModule(ShaderModuleState* state);

protected:
    struct ShaderJobEntry {
        DeviceDispatchTable *table;
        ShaderJob info;
    };

    /// Compile a given job
    void CompileShader(const ShaderJobEntry &job);

    /// Worker entry
    void Worker(void *userData);

private:
    DeviceDispatchTable* table{nullptr};

    /// Components
    ComRef<Dispatcher> dispatcher;
    ComRef<ShaderCompilerDebug> debug;
    ComRef<ShaderExportDescriptorAllocator> shaderExportDescriptorAllocator;

    /// All features
    std::vector<ComRef<IShaderFeature>> shaderFeatures;

    /// All data
    std::vector<ShaderDataInfo> shaderData;

    /// Number of exports
    uint32_t exportCount{0};
};
