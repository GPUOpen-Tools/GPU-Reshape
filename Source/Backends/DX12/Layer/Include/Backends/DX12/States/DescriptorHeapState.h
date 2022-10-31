#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
class ShaderExportDescriptorAllocator;
class PhysicalResourceMappingTable;

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

    /// Base addresses
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorBase{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorBase{};

    /// Stride of this heap
    uint64_t stride{0};

    /// Internal allocator
    ShaderExportDescriptorAllocator* allocator{nullptr};

    /// Mapping table
    PhysicalResourceMappingTable* prmTable{nullptr};
};
