#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/DeepCopy.Gen.h>
#include <Backends/DX12/InstrumentationInfo.h>

// Std
#include <atomic>
#include <vector>
#include <mutex>
#include <map>

// Forward declarations
struct DeviceState;
struct RootSignatureState;
struct ShaderState;

enum class PipelineType {
    None,
    Graphics,
    Compute
};

struct PipelineState {
    /// Parent state
    DeviceState* parent{};

    /// User pipeline
    ///  ! May be nullptr if the top pipeline has been destroyed
    ID3D12PipelineState* object{nullptr};

    /// Type of this pipeline
    PipelineType type{PipelineType::None};

    /// Replaced pipeline object, fx. instrumented version
    std::atomic<ID3D12PipelineState*> hotSwapObject{nullptr};

    /// Signature for this pipeline
    RootSignatureState* signature{nullptr};

    /// Referenced shaders
    ///   ! May not be indexed yet, indexing occurs during instrumentation.
    ///     Avoided during regular use to not tamper with performance.
    std::vector<ShaderState*> shaders;

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;

    /// Instrumented objects lookup
    /// TODO: How do we manage lifetimes here?
    std::map<uint64_t, ID3D12PipelineState*> instrumentObjects;

    /// Module specific lock
    std::mutex mutex;
};

struct GraphicsPipelineState : public PipelineState {
    /// Creation deep copy
    D3D12GraphicsPipelineStateDescDeepCopy deepCopy;
};

struct ComputePipelineState : public PipelineState {
    /// Creation deep copy
    D3D12ComputePipelineStateDescDeepCopy deepCopy;
};
