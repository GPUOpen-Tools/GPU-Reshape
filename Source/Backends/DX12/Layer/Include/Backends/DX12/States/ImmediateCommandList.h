#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

struct ImmediateCommandList {
    /// Immediate list
    ID3D12GraphicsCommandList* commandList{nullptr};

    /// Immediate allocator, tied to list
    ID3D12CommandAllocator* allocator{nullptr};
};
