#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
class ShaderExportDescriptorAllocator;
class PhysicalResourceMappingTable;
struct ResourceState;

struct __declspec(uuid("35585A4B-17E0-4D0C-BE86-D6CB806C93A5")) DescriptorHeapState {
    ~DescriptorHeapState();

    /// Check if a GPU handle is in bounds to this heap
    bool IsInBounds(D3D12_GPU_DESCRIPTOR_HANDLE handle) const {
        return handle.ptr >= gpuDescriptorBase.ptr && handle.ptr < gpuDescriptorBase.ptr + physicalDescriptorCount * stride;
    }

    /// Check if a CPU handle is in bounds to this heap
    bool IsInBounds(D3D12_CPU_DESCRIPTOR_HANDLE handle) const {
        return handle.ptr >= cpuDescriptorBase.ptr && handle.ptr < cpuDescriptorBase.ptr + physicalDescriptorCount * stride;
    }

    /// Get the state from a handle
    ResourceState* GetStateFromHeapHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

    /// Get the state from a handle
    ResourceState* GetStateFromHeapHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const;

    /// Parent state
    ID3D12Device* parent{};

    /// Owning allocator
    Allocators allocators;

    /// Is this heap exhausted? i.e. no injected entries
    bool exhausted{false};

    /// Type of this heap
    D3D12_DESCRIPTOR_HEAP_TYPE type;

    /// Flags of this heap
    D3D12_DESCRIPTOR_HEAP_FLAGS flags;

    /// Base addresses
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorBase{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorBase{};

    /// Stride of this heap
    uint64_t stride{0};

    /// Number of descriptors, includes prefix
    uint32_t physicalDescriptorCount{0};

    /// Internal allocator
    ShaderExportDescriptorAllocator* allocator{nullptr};

    /// Mapping table
    PhysicalResourceMappingTable* prmTable{nullptr};
};
