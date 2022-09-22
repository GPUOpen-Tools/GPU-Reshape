#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

struct DXGIFactoryState {
    ~DXGIFactoryState();

    /// Owning allocator
    Allocators allocators;
};
