#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

// Std
#include <vector>

struct SwapChainState {
    ~SwapChainState();

    /// Parent state
    ID3D12Device* parent{};

    /// Owning allocator
    Allocators allocators;

    /// Parent object
    IDXGISwapChain* object{};

    /// Wrapped buffers
    std::vector<ID3D12Resource*> buffers;
};
