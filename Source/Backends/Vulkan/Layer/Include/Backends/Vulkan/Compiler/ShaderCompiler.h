#pragma once

// Layer
#include <Backends/Vulkan/States/ShaderModuleState.h>

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

class ShaderCompiler : public TComponent<ShaderCompiler> {
public:
    COMPONENT(ShaderCompiler);

    ShaderCompiler(DeviceDispatchTable* table);

    /// Install this compiler
    bool Install();

    /// Add a shader job
    /// \param state the state to be compiled
    /// \param featureBitSet the enabled feature set
    /// \param bucket optional, the dispatcher bucket
    void Add(DeviceDispatchTable* table, ShaderModuleState *state, const ShaderModuleInstrumentationKey& instrumentationKey, DispatcherBucket *bucket = nullptr);

    /// Ensure a module is initialized
    /// \param state shader state
    void InitializeModule(ShaderModuleState* state);

protected:
    struct ShaderJob {
        DeviceDispatchTable *table;
        ShaderModuleState *state;
        ShaderModuleInstrumentationKey instrumentationKey;
    };

    /// Compile a given job
    void CompileShader(const ShaderJob &job);

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

    /// Number of exports
    uint32_t exportCount{0};
};
