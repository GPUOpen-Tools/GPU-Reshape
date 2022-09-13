#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Std
#include <vector>

// Forward declarations
struct DeviceState;

struct SwapChainState {
    /// Parent state
    DeviceState* parent{};

    /// Parent object
    IDXGISwapChain* object{};

    /// Wrapped buffers
    std::vector<ID3D12Resource*> buffers;
};
