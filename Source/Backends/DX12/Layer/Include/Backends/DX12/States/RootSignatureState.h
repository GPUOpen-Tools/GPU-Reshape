#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include "RootRegisterBindingInfo.h"

// Forward declarations
struct DeviceState;

struct RootSignatureState {
    /// Parent state
    DeviceState* parent{};

    /// Root binding information
    RootRegisterBindingInfo rootBindingInfo;

    /// Number of user parameters
    uint32_t userRootCount;
};
