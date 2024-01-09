// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/Config.h>
#include <Backends/DX12/Resource/VirtualResourceMapping.h>

// Common
#include <Common/Allocators.h>

// Forward declarations
class ShaderExportFixedTwoSidedDescriptorAllocator;
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

    /// Get the offset from a handle
    uint32_t GetOffsetFromHeapHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

    /// Get the offset from a handle
    uint32_t GetOffsetFromHeapHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const;

    /// Get the state from a handle
    ResourceState* GetStateFromHeapHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

    /// Get the state from a handle
    ResourceState* GetStateFromHeapHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const;

    /// Get the token from a handle
    VirtualResourceMapping GetVirtualMappingFromHeapHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

    /// Get the token from a handle
    VirtualResourceMapping GetVirtualMappingFromHeapHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const;

    /// Get the virtual bound
    uint32_t GetVirtualRangeBound() const {
#if DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
        return physicalDescriptorCount;
#else // DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
        return virtualDescriptorCount;
#endif // DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
    }

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

    /// Number of user descriptors
    uint32_t virtualDescriptorCount{0};

    /// Number of descriptors, includes prefix
    uint32_t physicalDescriptorCount{0};

    /// Internal allocator
    ShaderExportFixedTwoSidedDescriptorAllocator* allocator{nullptr};

    /// Mapping table
    PhysicalResourceMappingTable* prmTable{nullptr};
};
