#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
struct DeviceState;

struct CommandAllocatorState {
    ~CommandAllocatorState();

    /// Parent state
    DeviceState* parent{};

    /// Owning allocator
    Allocators allocators;
};
