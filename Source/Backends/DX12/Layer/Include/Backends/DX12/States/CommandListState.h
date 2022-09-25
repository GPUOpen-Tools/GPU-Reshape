#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
struct DeviceState;
struct ShaderExportStreamState;

struct CommandListState {
    ~CommandListState();

    /// Parent state
    ID3D12Device* parent{nullptr};

    /// Owning allocator
    Allocators allocators;

    /// Current streaming state
    ShaderExportStreamState* streamState{nullptr};

    /// Object
    ID3D12GraphicsCommandList* object{nullptr};
};
