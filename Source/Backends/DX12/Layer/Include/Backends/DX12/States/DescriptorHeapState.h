#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Forward declarations
struct DeviceState;
class ShaderExportDescriptorAllocator;

struct DescriptorHeapState {
    /// Parent state
    DeviceState* parent{};

    /// Is this heap exhausted? i.e. no injected entries
    bool exhausted{false};

    /// Type of this heap
    D3D12_DESCRIPTOR_HEAP_TYPE type;

    /// Internal allocator
    ShaderExportDescriptorAllocator* allocator{nullptr};
};
