// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/Export/ShaderExportDescriptorInfo.h>

// Common
#include <Common/Allocator/Vector.h>

// Std
#include <vector>

// Forward declarations
class ShaderExportHost;

class ShaderExportDescriptorAllocator {
public:
    /// Constructor
    /// \param device parent device
    /// \param heap target heap
    /// \param bound expected bound
    ShaderExportDescriptorAllocator(const Allocators& allocators, ID3D12Device* device, ID3D12DescriptorHeap* heap, uint32_t bound);

    /// Allocate a new descriptor
    /// \param count number of descriptors to allocate
    /// \return descriptor base info
    ShaderExportSegmentDescriptorInfo Allocate(uint32_t count);

    /// Free a descriptor
    /// \param id
    void Free(const ShaderExportSegmentDescriptorInfo& id);

    /// Get the allocation prefix
    uint32_t GetPrefix() const {
        return bound;
    }

    /// Get the allocation prefix offset
    uint64_t GetPrefixOffset() const {
        return bound * descriptorAdvance;
    }

    /// Get the descriptor ptr advance
    uint32_t GetAdvance() const {
        return descriptorAdvance;
    }

public:
    /// Get a safe bound
    /// \param host shader export host
    /// \return descriptor count bound
    static uint32_t GetDescriptorBound(ShaderExportHost* host);

private:
    /// Current allocation counter
    uint32_t slotAllocationCounter{0};

    /// Currently free descriptors
    Vector<uint32_t> freeDescriptors;

    /// Parent bound
    uint32_t bound;

    /// Parent heap
    ID3D12DescriptorHeap* heap;

    /// Heap advance
    uint32_t descriptorAdvance;

    /// Base CPU handle
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;

    /// Base GPU handle
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};

