#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/DeepCopy.Gen.h>
#include <Backends/DX12/InstrumentationInfo.h>

// Common
#include <Common/Containers/ReferenceObject.h>
#include <Common/Enum.h>
#include <Common/Allocators.h>

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
    None = 0,
    Graphics = BIT(1),
    Compute = BIT(2),
    Count = 2
};

BIT_SET(PipelineType);

struct PipelineState : public ReferenceObject {
    /// Reference counted destructor
    virtual ~PipelineState();

    /// Add an instrument to this module
    /// \param featureBitSet the enabled feature set
    /// \param pipeline the pipeline in question
    void AddInstrument(uint64_t featureBitSet, ID3D12PipelineState* pipeline) {
        std::lock_guard lock(mutex);
        instrumentObjects[featureBitSet] = pipeline;
    }

    /// Get an instrument
    /// \param featureBitSet the enabled feature set
    /// \return nullptr if not found
    ID3D12PipelineState* GetInstrument(uint64_t featureBitSet) {
        std::lock_guard lock(mutex);
        auto&& it = instrumentObjects.find(featureBitSet);
        if (it == instrumentObjects.end()) {
            return nullptr;
        }

        return it->second;
    }

    /// Parent state
    DeviceState* parent{};

    /// Owning allocator
    Allocators allocators;

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
    std::vector<ShaderState*> shaders;

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;

    /// Instrumented objects lookup
    /// TODO: How do we manage lifetimes here?
    std::map<uint64_t, ID3D12PipelineState*> instrumentObjects;

    /// Unique ID
    uint64_t uid{0};

    /// Module specific lock
    std::mutex mutex;
};

struct GraphicsPipelineState : public PipelineState {
    /// Creation deep copy
    D3D12GraphicsPipelineStateDescDeepCopy deepCopy;

    /// Stage shaders
    ShaderState* vs{nullptr};
    ShaderState* hs{nullptr};
    ShaderState* ds{nullptr};
    ShaderState* gs{nullptr};
    ShaderState* ps{nullptr};
};

struct ComputePipelineState : public PipelineState {
    /// Creation deep copy
    D3D12ComputePipelineStateDescDeepCopy deepCopy;

    /// Stage shaders
    ShaderState* cs{nullptr};
};
