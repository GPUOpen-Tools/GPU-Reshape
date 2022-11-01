#pragma once

// Layer
#include <Backends/DX12/States/ShaderState.h>

// Backend
#include <Backend/Resource/ShaderResourceInfo.h>

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
class ShaderCompilerDebug;
class DXILSigner;
class DXBCSigner;

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

    /// Ensure a module is initialized
    /// \param state shader state
    void InitializeModule(ShaderState* state);

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

    /// Debug device
    ComRef<ShaderCompilerDebug> debug;

    /// Signers
    ComRef<DXILSigner> dxilSigner;
    ComRef<DXBCSigner> dxbcSigner;

    /// All features
    std::vector<ComRef<IShaderFeature>> shaderFeatures;

    /// All resources
    std::vector<ShaderResourceInfo> resources;

    /// Number of exports
    uint32_t exportCount{0};
};
