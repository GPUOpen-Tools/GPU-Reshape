#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/Resource/VirtualResourceMapping.h>

// Common
#include <Common/Allocators.h>

struct __declspec(uuid("09175D5B-BA8A-4531-9553-BC1CD024A1FE")) ResourceState {
    ~ResourceState();

    /// Parent state
    ID3D12Device* parent{};

    /// Owning allocator
    Allocators allocators;

    /// Resource mapping
    VirtualResourceMapping virtualMapping;
};
