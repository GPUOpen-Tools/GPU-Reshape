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
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/Export/ShaderExportDescriptorInfo.h>

// Common
#include <Common/Allocator/Vector.h>

// Std
#include <vector>

// Forward declarations
class ShaderExportHost;

class ShaderExportFixedTwoSidedDescriptorAllocator {
public:
    /// Constructor
    /// \param device parent device
    /// \param heap target heap
    /// \param bound expected bound
    ShaderExportFixedTwoSidedDescriptorAllocator(ID3D12Device* device, ID3D12DescriptorHeap* heap, uint32_t lhsWidth, uint32_t rhsWidth, uint32_t offset, uint32_t bound);

    /// Allocate a new descriptor
    /// \param width number of descriptors to allocate
    /// \return descriptor base info
    ShaderExportSegmentDescriptorInfo Allocate(uint32_t width);

    /// Free a descriptor
    /// \param id
    void Free(const ShaderExportSegmentDescriptorInfo& id);

    /// Get the allocation prefix
    uint32_t GetPrefix() const {
#if DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
        return bound;
#else // DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
        return 0u;
#endif // DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
    }

    /// Get the allocation prefix offset
    uint64_t GetPrefixOffset() const {
#if DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
        return bound * static_cast<uint32_t>(lhsBucket.descriptorAdvance);
#else // DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
        return 0u;
#endif // DESCRIPTOR_HEAP_METHOD == DESCRIPTOR_HEAP_METHOD_PREFIX
    }

    /// Get the descriptor ptr advance
    uint32_t GetAdvance() const {
        return static_cast<uint32_t>(lhsBucket.descriptorAdvance);
    }

public:
    /// Get a safe bound
    /// \param host shader export host
    /// \return descriptor count bound
    static uint32_t GetDescriptorBound(ShaderExportHost* host);

private:
    struct AllocationBucket {
        /// Bucket width
        uint32_t width{0};
        
        /// Current allocation counter
        uint32_t slotAllocationCounter{0};

        /// Heap advance
        int64_t descriptorAdvance;
        
        /// Currently free descriptors
        std::vector<uint32_t> freeDescriptors;

        /// Base CPU handle
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;

        /// Base GPU handle
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
    };

    /// Get the forward bucket for allocations
    AllocationBucket& GetForwardBucket(uint32_t width);

    // Get the backwards bucket for complimentary checks
    AllocationBucket& GetBackwardBucket(uint32_t width);

    /// Double sided buckets
    AllocationBucket lhsBucket;
    AllocationBucket rhsBucket;

private:
    /// Parent bound
    uint32_t bound;

    /// Parent heap
    ID3D12DescriptorHeap* heap;
};

