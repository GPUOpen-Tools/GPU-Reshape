#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/Resource/VirtualResourceMapping.h>

// Common
#include <Common/Allocators.h>

struct ResourceState {
    ~ResourceState();

    /// Parent state
    ID3D12Device* parent{};

    /// Owning allocator
    Allocators allocators;

    /// Resource mapping
    VirtualResourceMapping virtualMapping;
};
