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

#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/Export/ShaderExportOwnedHeapAllocator.h>

ShaderExportOwnedHeapAllocation ShaderExportOwnedHeapAllocator::Allocate(DeviceState* deviceState, uint32_t length) {
    // Out of descriptor space?
    if (segments.empty() || !segments.back().CanAccomodate(length)) {
        // Conservative growth
        const uint32_t lastDescriptorCount = segments.empty() ? 1024 : segments.back().count;
        const uint32_t descriptorCount = static_cast<uint32_t>(static_cast<float>(std::max<size_t>(length, lastDescriptorCount)) * 1.5f);

        // Create new segment
        ShaderExportOwnedHeapSegment& segment = segments.emplace_back();
        segment.stride = deviceState->object->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        segment.count = descriptorCount;

        // Create new heap
        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = descriptorCount;
        deviceState->object->CreateDescriptorHeap(&desc, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&segment.heap));
    }

    // Current segment
    ShaderExportOwnedHeapSegment& segment = segments.back();

    // Descriptor offset
    uint64_t offset = segment.head * segment.stride;

    // Offset head
    segment.head += length;

    // Create view
    return {
        .heap = segment.heap,
        .stride = segment.stride,
        .cpu = {segment.heap->GetCPUDescriptorHandleForHeapStart().ptr + offset},
        .gpu = {segment.heap->GetGPUDescriptorHandleForHeapStart().ptr + offset}
    };
}
