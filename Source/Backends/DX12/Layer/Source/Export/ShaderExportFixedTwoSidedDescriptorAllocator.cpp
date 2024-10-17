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

#include <Backends/DX12/Export/ShaderExportFixedTwoSidedDescriptorAllocator.h>
#include <Backends/DX12/Export/ShaderExportHost.h>

// Backend
#include <Backend/Diagnostic/DiagnosticFatal.h>

// Std
#include <algorithm>

ShaderExportFixedTwoSidedDescriptorAllocator::ShaderExportFixedTwoSidedDescriptorAllocator(ID3D12Device *device, ID3D12DescriptorHeap *heap, uint32_t lhsWidth, uint32_t rhsWidth, uint32_t offset, uint32_t bound) : bound(bound), heap(heap) {
    // Get the advance
    auto advance = static_cast<int64_t>(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    // Is GPU visible?
    bool isGPUVisible = heap->GetDesc().Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    
    // Left bucket, advance right
    lhsBucket.width = lhsWidth;
    lhsBucket.cpuHandle = { heap->GetCPUDescriptorHandleForHeapStart().ptr + advance * offset };
    lhsBucket.gpuHandle = D3D12_GPU_DESCRIPTOR_HANDLE {};
    lhsBucket.descriptorAdvance = advance;

    // GPU handle optional
    if (isGPUVisible) {
        lhsBucket.gpuHandle = { heap->GetGPUDescriptorHandleForHeapStart().ptr + advance * offset };
    }

    // Right bucket, advance left
    rhsBucket.width = rhsWidth;
    rhsBucket.cpuHandle = { lhsBucket.cpuHandle.ptr + advance * bound };
    rhsBucket.gpuHandle = { lhsBucket.gpuHandle.ptr + advance * bound };
    rhsBucket.descriptorAdvance = -advance;
}

uint32_t ShaderExportFixedTwoSidedDescriptorAllocator::GetDescriptorBound(ShaderExportHost *host) {
    // Entirely unsafe number of simultaneously executing command lists
    const uint32_t kMaxExecutingLists = 16384;

    // Number of exports
    uint32_t count;
    host->Enumerate(&count, nullptr);

    // Descriptor estimate
    return count * kMaxExecutingLists;
}

ShaderExportFixedTwoSidedDescriptorAllocator::AllocationBucket & ShaderExportFixedTwoSidedDescriptorAllocator::GetForwardBucket(uint32_t width) {
    ASSERT(width == lhsBucket.width || width == rhsBucket.width, "Invalid width");
    return width == lhsBucket.width ? lhsBucket : rhsBucket;
}

ShaderExportFixedTwoSidedDescriptorAllocator::AllocationBucket & ShaderExportFixedTwoSidedDescriptorAllocator::GetBackwardBucket(uint32_t width) {
    ASSERT(width == lhsBucket.width || width == rhsBucket.width, "Invalid width");
    return width == lhsBucket.width ? rhsBucket : lhsBucket;
}

ShaderExportSegmentDescriptorInfo ShaderExportFixedTwoSidedDescriptorAllocator::Allocate(uint32_t width) {
    AllocationBucket& bucket = GetForwardBucket(width);
    
    // Any free'd?
    if (!bucket.freeDescriptors.empty()) {
        uint32_t id = bucket.freeDescriptors.back();
        bucket.freeDescriptors.pop_back();

        // Setup allocation
        ShaderExportSegmentDescriptorInfo info;
#ifndef NDEBUG
        info.heap = heap;
#endif // NDEBUG
        info.width = width;
        info.offset = id;
        info.cpuHandle.ptr = bucket.cpuHandle.ptr + info.offset * bucket.descriptorAdvance;
        info.gpuHandle.ptr = bucket.gpuHandle.ptr + info.offset * bucket.descriptorAdvance;
        
        // Rhs space is shifted by the width
        if (bucket.descriptorAdvance < 0) {
            info.cpuHandle.ptr += width * bucket.descriptorAdvance;
            info.gpuHandle.ptr += width * bucket.descriptorAdvance;
        }

        // OK
        return info;
    }

    // Out of descriptors?
    if (lhsBucket.slotAllocationCounter + rhsBucket.slotAllocationCounter + width > bound) {
        // Display friendly message
        Backend::DiagnosticFatal(
            "Two-Sided Descriptor Exhaustion",
            "GPU Reshape has run out internal descriptors for command list patching. Please report this issue."
        );

        // Unreachable
        return {};
    }

    // Setup allocation
    ShaderExportSegmentDescriptorInfo info;
#ifndef NDEBUG
    info.heap = heap;
#endif // NDEBUG
    info.width = width;
    info.offset = bucket.slotAllocationCounter;
    info.cpuHandle.ptr = bucket.cpuHandle.ptr + info.offset * bucket.descriptorAdvance;
    info.gpuHandle.ptr = bucket.gpuHandle.ptr + info.offset * bucket.descriptorAdvance;

    // Rhs space is shifted by the width
    if (bucket.descriptorAdvance < 0) {
        info.cpuHandle.ptr += width * bucket.descriptorAdvance;
        info.gpuHandle.ptr += width * bucket.descriptorAdvance;
    }

    // Advance
    bucket.slotAllocationCounter += width;

    // Two sided allocator may overwrite the free'd ranges, this requires culling
    AllocationBucket& complimentaryBucket = GetBackwardBucket(width);

    // Get the upper bound to the inverted range
    auto it = std::upper_bound(
        complimentaryBucket.freeDescriptors.begin(),
        complimentaryBucket.freeDescriptors.end(),
        bound - bucket.slotAllocationCounter
    );

    // Upper bound is inclusive, any allocation past this is invalidated
    if (it != complimentaryBucket.freeDescriptors.end()) {
        complimentaryBucket.freeDescriptors.erase(it, complimentaryBucket.freeDescriptors.end());
    }

    // OK
    return info;
}

void ShaderExportFixedTwoSidedDescriptorAllocator::Free(const ShaderExportSegmentDescriptorInfo& id) {
    AllocationBucket& bucket = GetForwardBucket(id.width);

    // Validate
#ifndef NDEBUG
    ASSERT(id.heap == heap, "Mismatched heap in shader export descriptor free");
#endif // NDEBUG

    // Append as free, insert sorted
    bucket.freeDescriptors.insert(std::upper_bound(bucket.freeDescriptors.begin(), bucket.freeDescriptors.end(), id.offset), id.offset);
}
