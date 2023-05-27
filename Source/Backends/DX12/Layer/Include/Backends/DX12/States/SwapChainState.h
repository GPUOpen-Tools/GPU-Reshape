#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

// Std
#include <vector>
#include <chrono>

struct __declspec(uuid("664ECE39-6CD9-49E1-9790-21464F3F450A")) SwapChainState {
    SwapChainState(const Allocators& allocators) : buffers(allocators) {
        
    }
    
    ~SwapChainState();

    /// Parent state
    ID3D12Device* parent{};

    /// Owning allocator
    Allocators allocators;

    /// Parent object
    IDXGISwapChain* object{};

    /// Last present time
    std::chrono::time_point<std::chrono::high_resolution_clock> lastPresentTime = std::chrono::high_resolution_clock::now();

    /// Wrapped buffers
    Vector<ID3D12Resource*> buffers;
};
