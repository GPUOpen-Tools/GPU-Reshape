#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Forward declarations
struct DeviceState;

struct CommandAllocatorState {
    /// Parent state
    DeviceState* parent{};
};
