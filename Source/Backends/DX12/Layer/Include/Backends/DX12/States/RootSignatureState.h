#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Forward declarations
struct DeviceState;

struct RootSignatureState {
    /// Parent state
    DeviceState* parent{};

    /// Number of user provided root constants
    uint32_t userRootCount{0};
};
