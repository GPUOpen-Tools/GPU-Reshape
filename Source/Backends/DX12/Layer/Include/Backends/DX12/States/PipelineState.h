#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/DeepCopy.Gen.h>
#include <Backends/DX12/InstrumentationInfo.h>
#include "PipelineType.h"

// Common
#include <Common/Allocator/Vector.h>
#include <Common/Containers/ReferenceObject.h>
#include <Common/Enum.h>
#include <Common/Allocators.h>

// Std
#include <atomic>
#include <vector>
#include <mutex>
#include <map>

// Forward declarations
struct RootSignatureState;
struct ShaderState;

struct __declspec(uuid("7C251A06-33FD-42DF-8850-40C1077FCAFE")) PipelineState : public ReferenceObject {
    PipelineState(const Allocators& allocators) : shaders(allocators), subObjectStreamBlob(allocators) {
        
    }
    
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
    ID3D12Device* parent{};

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
    Vector<ShaderState*> shaders;

    /// Optional debug name
    char* debugName{nullptr};

    /// Instrumentation info
    InstrumentationInfo instrumentationInfo;

    /// Instrumented objects lookup
    /// TODO: How do we manage lifetimes here?
    std::map<uint64_t, ID3D12PipelineState*> instrumentObjects;

    /// Optional pipeline stream blob
    Vector<uint8_t> subObjectStreamBlob;

    /// Unique ID
    uint64_t uid{0};

    /// Module specific lock
    std::mutex mutex;
};

struct GraphicsPipelineState : public PipelineState {
    using PipelineState::PipelineState;
    
    /// Creation deep copy, if invalid, present in stream blob
    D3D12GraphicsPipelineStateDescDeepCopy deepCopy;

    /// Stage shaders
    ShaderState* vs{nullptr};
    ShaderState* hs{nullptr};
    ShaderState* ds{nullptr};
    ShaderState* gs{nullptr};
    ShaderState* ps{nullptr};

    /// Stream offsets
    uint64_t streamVSOffset{0};
    uint64_t streamHSOffset{0};
    uint64_t streamDSOffset{0};
    uint64_t streamGSOffset{0};
    uint64_t streamPSOffset{0};
};

struct ComputePipelineState : public PipelineState {
    using PipelineState::PipelineState;
    
    /// Creation deep copy, if invalid, present in stream blob
    D3D12ComputePipelineStateDescDeepCopy deepCopy;

    /// Stage shaders
    ShaderState* cs{nullptr};

    /// Stream offsets
    uint64_t streamCSOffset{0};
};
