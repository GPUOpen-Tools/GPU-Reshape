#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

struct __declspec(uuid("005659C7-4B20-4BD7-A632-1C3A55B5F1F3")) DXGIFactoryState {
    ~DXGIFactoryState();

    /// Owning allocator
    Allocators allocators;
};
