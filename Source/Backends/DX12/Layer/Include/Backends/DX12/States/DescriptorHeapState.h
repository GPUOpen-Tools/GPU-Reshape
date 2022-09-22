#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
struct DeviceState;
class ShaderExportDescriptorAllocator;

struct DescriptorHeapState {
    ~DescriptorHeapState();

    /// Parent state
    DeviceState* parent{};

    /// Owning allocator
    Allocators allocators;

    /// Is this heap exhausted? i.e. no injected entries
    bool exhausted{false};

    /// Type of this heap
    D3D12_DESCRIPTOR_HEAP_TYPE type;

    /// Internal allocator
    ShaderExportDescriptorAllocator* allocator{nullptr};
};
