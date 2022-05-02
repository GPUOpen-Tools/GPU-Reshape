#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Forward declarations
struct DeviceState;

struct CommandListState {
    /// Parent state
    DeviceState* parent{};
};

struct GraphicsCommandListState : public CommandListState {
    /// Parent state
    DeviceState* parent{};
};

struct ComputeCommandListState : public CommandListState {
    /// Parent state
    DeviceState* parent{};
};
