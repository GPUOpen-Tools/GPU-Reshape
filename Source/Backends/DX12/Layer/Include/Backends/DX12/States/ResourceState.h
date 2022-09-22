#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
struct DeviceState;

struct ResourceState {
    ~ResourceState();

    /// Parent state
    DeviceState* parent{};

    /// Owning allocator
    Allocators allocators;
};
