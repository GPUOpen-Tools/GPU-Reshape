#pragma once

// Layer
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Compiler/ShaderCompilerDiagnostic.h>

// Backend
#include <Backend/ShaderData/ShaderDataInfo.h>

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

struct ShaderJob {
    /// State to compile
    ShaderState* state{ nullptr };

    /// Instrumentation key to apply
    ShaderInstrumentationKey instrumentationKey;

    /// Optional diagnostics
    ShaderCompilerDiagnostic* diagnostic{nullptr};

    /// Pipeline dependent specialization info
    MessageStream* dependentSpecialization{nullptr};
};

class ShaderCompiler : public TComponent<ShaderCompiler> {
public:
    COMPONENT(ShaderCompiler);

    ShaderCompiler(DeviceState* device);

    /// Install this compiler
    bool Install();

    /// Add a shader job
    /// \param job job to enqueue
    /// \param bucket optional, the dispatcher bucket
    void Add(const ShaderJob& job, DispatcherBucket *bucket = nullptr);

    /// Ensure a module is initialized
    /// \param state shader state
    bool InitializeModule(ShaderState* state);

protected:
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
    Vector<ComRef<IShaderFeature>> shaderFeatures;

    /// All data
    Vector<ShaderDataInfo> shaderData;

    /// Number of exports
    uint32_t exportCount{0};
};
