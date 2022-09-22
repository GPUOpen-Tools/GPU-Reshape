#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

// Std
#include <vector>

// Forward declarations
struct DeviceState;

struct SwapChainState {
    ~SwapChainState();

    /// Parent state
    DeviceState* parent{};

    /// Owning allocator
    Allocators allocators;

    /// Parent object
    IDXGISwapChain* object{};

    /// Wrapped buffers
    std::vector<ID3D12Resource*> buffers;
};
