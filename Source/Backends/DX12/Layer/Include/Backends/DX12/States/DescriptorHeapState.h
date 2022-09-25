#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
class ShaderExportDescriptorAllocator;

struct DescriptorHeapState {
    ~DescriptorHeapState();

    /// Parent state
    ID3D12Device* parent{};

    /// Owning allocator
    Allocators allocators;

    /// Is this heap exhausted? i.e. no injected entries
    bool exhausted{false};

    /// Type of this heap
    D3D12_DESCRIPTOR_HEAP_TYPE type;

    /// Internal allocator
    ShaderExportDescriptorAllocator* allocator{nullptr};
};
