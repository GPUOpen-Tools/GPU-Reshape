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

#include <Backends/DX12/Export/ShaderExportDescriptorAllocator.h>
#include <Backends/DX12/Export/ShaderExportHost.h>

ShaderExportDescriptorAllocator::ShaderExportDescriptorAllocator(const Allocators& allocators, ID3D12Device* device, ID3D12DescriptorHeap *heap, uint32_t bound) : freeDescriptors(allocators), bound(bound), heap(heap) {
    D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();

    // CPU base
    cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();

    // GPU base
    if (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
        gpuHandle = heap->GetGPUDescriptorHandleForHeapStart();
    }

    descriptorAdvance = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

uint32_t ShaderExportDescriptorAllocator::GetDescriptorBound(ShaderExportHost *host) {
    // Entirely unsafe number of simultaneously executing command lists
    const uint32_t kMaxExecutingLists = 16384;

    // Number of exports
    uint32_t count;
    host->Enumerate(&count, nullptr);

    // Descriptor estimate
    return count * kMaxExecutingLists;
}

ShaderExportSegmentDescriptorInfo ShaderExportDescriptorAllocator::Allocate(uint32_t count) {
    // Any free'd?
    if (!freeDescriptors.empty()) {
        uint32_t id = freeDescriptors.back();
        freeDescriptors.pop_back();

        ShaderExportSegmentDescriptorInfo info;
#ifndef NDEBUG
        info.heap = heap;
#endif // NDEBUG
        info.offset = id;
        info.cpuHandle.ptr = cpuHandle.ptr + info.offset * descriptorAdvance;
        info.gpuHandle.ptr = gpuHandle.ptr + info.offset * descriptorAdvance;
        return info;
    }

    // Out of descriptors?
    if (slotAllocationCounter + (count - 1) >= bound) {
        return {};
    }

    // Next!
    ShaderExportSegmentDescriptorInfo info;
#ifndef NDEBUG
    info.heap = heap;
#endif // NDEBUG
    info.offset = slotAllocationCounter;
    info.cpuHandle.ptr = cpuHandle.ptr + info.offset * descriptorAdvance;
    info.gpuHandle.ptr = gpuHandle.ptr + info.offset * descriptorAdvance;

    slotAllocationCounter += count;

    return info;
}

void ShaderExportDescriptorAllocator::Free(const ShaderExportSegmentDescriptorInfo& id) {
#ifndef NDEBUG
    ASSERT(id.heap == heap, "Mismatched heap in shader export descriptor free");
#endif // NDEBUG
    freeDescriptors.push_back(id.offset);
}
