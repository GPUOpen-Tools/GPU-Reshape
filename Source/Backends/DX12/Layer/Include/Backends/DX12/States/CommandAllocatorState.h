#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

struct CommandAllocatorState {
    ~CommandAllocatorState();

    /// Parent state
    ID3D12Device* parent{};

    /// User command list type
    D3D12_COMMAND_LIST_TYPE userType = D3D12_COMMAND_LIST_TYPE_DIRECT;

    /// Owning allocator
    Allocators allocators;
};
