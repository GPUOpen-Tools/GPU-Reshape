#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/DeepCopy.Gen.h>

// Forward declarations
struct DeviceState;

enum class PipelineType {
    None,
    Graphics,
    Compute
};

struct PipelineState {
    /// Parent state
    DeviceState* parent{};

    /// Type of this pipeline
    PipelineType type{PipelineType::None};
};

struct GraphicsPipelineState : public PipelineState {
    /// Creation deep copy
    D3D12GraphicsPipelineStateDescDeepCopy deepCopy;
};

struct ComputePipelineState : public PipelineState {
    /// Creation deep copy
    D3D12ComputePipelineStateDescDeepCopy deepCopy;
};
