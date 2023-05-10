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

    /// User object
    ID3D12Resource* object{nullptr};

    /// Owning allocator
    Allocators allocators;

    /// Resource creation
    D3D12_RESOURCE_DESC desc;

    /// Optional debug name
    char* debugName{nullptr};

    /// Resource mapping
    VirtualResourceMapping virtualMapping;

    /// Unique ID
    uint64_t uid{0};
};
