#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/TrackedObject.h>
#include <Backends/DX12/DependentObject.h>
#include <Backends/DX12/Compiler/ShaderSet.h>

// Backend
#include <Backend/Environment.h>

// Common
#include <Common/ComRef.h>
#include <Common/Registry.h>
#include <Common/Containers/SlotArray.h>

// Forward declarations
class ShaderSet;
class IFeature;
struct ShaderState;
struct PipelineState;
class InstrumentationController;
class IBridge;

struct DeviceState {
    /// Owned object
    ID3D12Device* object{nullptr};

    /// Shared allocators
    Allocators allocators;

    /// Shared registry
    Registry registry;

    /// All shared shader sets
    ShaderSet shaderSet;

    /// Message bridge
    ComRef<IBridge> bridge;

    /// Tracked objects
    TrackedObject<ShaderState> states_Shaders;
    TrackedObject<PipelineState> states_Pipelines;

    /// Dependency objects
    DependentObject<ShaderState, PipelineState> dependencies_shaderPipelines;

    /// Controllers
    ComRef<InstrumentationController> instrumentationController{nullptr};

    /// Optional environment, ignored if creation parameters supply a registry
    Backend::Environment environment;

    /// All features
    std::vector<ComRef<IFeature>> features;
};
