#pragma once

// Layer
#include <Backends/Vulkan/ShaderModuleState.h>

// Common
#include <Common/IComponent.h>

// Std
#include <mutex>

class Dispatcher;
struct DispatcherBucket;
class IFeature;
class IShaderFeature;

class ShaderCompiler : public IComponent {
public:
    COMPONENT(ShaderCompiler);

    /// Install this compiler
    bool Install();

    /// Add a shader job
    /// \param state the state to be compiled
    /// \param featureBitSet the enabled feature set
    /// \param bucket optional, the dispatcher bucket
    void Add(DeviceDispatchTable* table, ShaderModuleState *state, uint64_t featureBitSet, DispatcherBucket *bucket = nullptr);

protected:
    struct ShaderJob {
        DeviceDispatchTable *table;
        ShaderModuleState *state;
        uint64_t featureBitSet;
    };

    /// Compile a given job
    void CompileShader(const ShaderJob &job);

    /// Worker entry
    void Worker(void *userData);

private:
    /// Async dispatcher
    Dispatcher *dispatcher{nullptr};

    /// All features
    std::vector<IFeature *> features;
    std::vector<IShaderFeature *> shaderFeatures;
};
