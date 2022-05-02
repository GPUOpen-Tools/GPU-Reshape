#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Forward declarations
struct DeviceState;

struct CommandQueueState {
    /// Parent state
    DeviceState* parent{};
};
