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
#include <Backends/Vulkan/VMA.h>
#include <Backends/Vulkan/Allocation/Allocation.h>
#include <Backends/Vulkan/Allocation/MirrorAllocation.h>
#include <Backends/Vulkan/Allocation/Residency.h>

// Common
#include <Common/IComponent.h>

// Forward declarations
struct DeviceDispatchTable;

/// Handles device memory allocations and binding
class DeviceAllocator : public TComponent<DeviceAllocator> {
public:
    COMPONENT(DeviceAllocator);

    ~DeviceAllocator();

    /// Install this allocator
    /// \param table
    void Install(DeviceDispatchTable* table);

    /// Create a new allocation
    /// \param size byte size of the allocation
    /// \param residency expected residency of the allocation
    /// \return the allocation
    Allocation Allocate(const VkMemoryRequirements& requirements, AllocationResidency residency = AllocationResidency::Device);

    /// Create a new mirror allocation
    /// \param size byte size of the allocation
    /// \param residency expected residency of the allocation, if Host, the allocations are shared
    /// \return the mirror allocation
    MirrorAllocation AllocateMirror(const VkMemoryRequirements& requirements, AllocationResidency residency = AllocationResidency::Device);

    /// Free a given allocation
    /// \param allocation
    void Free(const Allocation& allocation);

    /// Free a given mirror allocation
    /// \param mirrorAllocation
    void Free(const MirrorAllocation& mirrorAllocation);

    /// Bind a buffer allocation
    /// \param allocation the allocation to be bound
    /// \param buffer the buffer to bind with
    /// \return success state
    bool BindBuffer(const Allocation& allocation, VkBuffer buffer);

    /// Map an allocation
    /// \param allocation the allocation to be mapped
    /// \return mapped address, nullptr if failed
    void* Map(const Allocation& allocation);

    /// Unmap a mapped allocation
    /// \param allocation the mapped allocation
    void Unmap(const Allocation& allocation);

    /// Flush a mapped allocation
    /// \param allocation the mapped allocation
    /// \param offset byte offset
    /// \param length byte length
    void FlushMappedRange(const Allocation& allocation, uint64_t offset, uint64_t length);

private:
    VmaAllocator allocator{nullptr};
};