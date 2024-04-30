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

// Backend
#include "VirtualResourceMapping.h"
#include <Backends/DX12/Allocation/MirrorAllocation.h>

// Common
#include <Common/Containers/Vector.h>
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
class DeviceAllocator;
struct ResourceState;

/// Performs mapping between virtual heaps and physical resources
class PhysicalResourceMappingTable {
public:
    PhysicalResourceMappingTable(const Allocators& allocators, const ComRef<DeviceAllocator>& allocator);

    /// Destructor
    ~PhysicalResourceMappingTable();

    /// Install the table
    /// \param type type of the descriptors
    /// \param count number of descriptors
    void Install(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);

    /// Update the table for use on a given list
    /// \param list list to be updated on
    void Update(ID3D12GraphicsCommandList* list);

    /// Write a single mapping at a given offset
    /// \param offset offset to be written
    /// \param mapping mapping to write
    void WriteMapping(uint32_t offset, const VirtualResourceMapping& mapping);

    /// Write a single mapping at a given offset
    /// \param offset offset to be written
    /// \param state given state
    /// \param mapping mapping to write
    void WriteMapping(uint32_t offset, ResourceState* state, const VirtualResourceMapping& mapping);

    /// Copy a mapping
    /// \param source source offset
    /// \param dest dest offset
    void CopyMapping(uint32_t source, uint32_t dest);

    /// Set the state of a mapping
    /// \param offset offset to be written
    /// \param state given state
    void SetMappingState(uint32_t offset, ResourceState* state);

    /// Get the mapping from an offset
    /// \param offset given offset
    /// \return state, nullptr if not found
    ResourceState* GetMappingState(uint32_t offset);

    /// Get the mapping from an offset
    /// \param offset given offset
    /// \return mapping
    VirtualResourceMapping GetMapping(uint32_t offset);

    /// Get the mapping from an offset
    /// \param offset given offset
    /// \param state the destination state
    /// \return mapping
    VirtualResourceMapping GetMapping(uint32_t offset, ResourceState** state);

    /// Get the underlying resource
    /// \return
    ID3D12Resource* GetResource() const {
        return allocation.device.resource;
    }

    /// Get the descriptor view
    /// \return
    const D3D12_SHADER_RESOURCE_VIEW_DESC& GetView() const {
        return view;
    }

private:
    /// Does this table need updating?
    bool isDirty{true};

    /// Number of mappings contained
    uint32_t virtualMappingCount{0};

    /// Mapped virtual entries
    VirtualResourceMapping* virtualMappings{nullptr};

    /// Underlying allocation
    MirrorAllocation allocation;

    /// Allocation view
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};

private:
    /// Underlying heap type
    D3D12_DESCRIPTOR_HEAP_TYPE type;
    
    /// All states
    Vector<ResourceState*> states;

    /// Shared lock
    std::mutex mutex;

private:
    ComRef<DeviceAllocator> allocator{};
};
