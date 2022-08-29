#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Forward declarations
struct DeviceState;
struct ShaderExportStreamState;

struct CommandListState {
    /// Parent state
    DeviceState* parent{nullptr};

    /// Current streaming state
    ShaderExportStreamState* streamState{nullptr};

    /// Object
    ID3D12GraphicsCommandList* object{nullptr};
};
