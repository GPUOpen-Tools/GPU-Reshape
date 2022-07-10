#pragma once

// Layer
#include <Backends/DX12/States/ShaderState.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <mutex>

// Forward declarations
class Dispatcher;
struct DeviceState;
struct DispatcherBucket;
class IFeature;
class IShaderFeature;

class ShaderCompiler : public TComponent<ShaderCompiler> {
public:
    COMPONENT(ShaderCompiler);

    ShaderCompiler(DeviceState* device);

    /// Install this compiler
    bool Install();

    /// Add a shader job
    /// \param state the state to be compiled
    /// \param featureBitSet the enabled feature set
    /// \param bucket optional, the dispatcher bucket
    void Add(ShaderState* state, const ShaderInstrumentationKey& instrumentationKey, DispatcherBucket *bucket = nullptr);

protected:
    struct ShaderJob {
        ShaderState *state;
        ShaderInstrumentationKey instrumentationKey;
    };

    /// Compile a given job
    void CompileShader(const ShaderJob &job);

    /// Worker entry
    void Worker(void *userData);

private:
    DeviceState* device;

    /// Async dispatcher
    ComRef<Dispatcher> dispatcher;

    /// All features
    std::vector<ComRef<IShaderFeature>> shaderFeatures;

    /// Number of exports
    uint32_t exportCount{0};
};
