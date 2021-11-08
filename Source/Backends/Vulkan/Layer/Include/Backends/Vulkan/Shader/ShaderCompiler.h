#pragma once

#include <Backends/Vulkan/ShaderModuleState.h>

#include <Common/IComponent.h>

#include <mutex>

class Dispatcher;
struct DispatcherBucket;
class IFeature;

class ShaderCompiler : public IComponent {
public:
    COMPONENT(ShaderCompiler);

    /// Initialize this compiler
    bool Initialize();

    /// Add a shader job
    /// \param state the state to be compiled
    /// \param featureBitSet the enabled feature set
    /// \param bucket optional, the dispatcher bucket
    void Add(ShaderModuleState* state, uint64_t featureBitSet, DispatcherBucket* bucket = nullptr);

protected:
    struct ShaderJob {
        ShaderModuleState* state;
        uint64_t           featureBitSet;
    };

    /// Compile a given job
    void CompileShader(const ShaderJob& job);

    /// Worker entry
    void Worker(void* userData);

private:
    /// Async dispatcher
    Dispatcher* dispatcher{nullptr};

    /// All features
    std::vector<IFeature*> features;
};
