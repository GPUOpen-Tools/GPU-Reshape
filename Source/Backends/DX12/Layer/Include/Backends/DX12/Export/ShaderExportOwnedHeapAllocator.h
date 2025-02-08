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

// Std
#include <vector>

// Forward declarations
struct DeviceState;

struct ShaderExportOwnedHeapSegment {
    /// Can this segment accomodate for a given set of descriptors?
    bool CanAccomodate(size_t length) {
        return head + length <= count;
    }

    /// Underlying heap
    ID3D12DescriptorHeap* heap{nullptr};

    /// Total number of descriptors
    uint32_t count{0};

    /// Physical stride between each descriptor
    uint32_t stride{0};

    /// Current allocation offset
    uint32_t head{0};
};

struct ShaderExportOwnedHeapAllocation {
    /// Get the GPU handle for an offset
    D3D12_CPU_DESCRIPTOR_HANDLE CPU(uint32_t offset = 0) const {
        return {cpu.ptr + offset * stride};
    }
    
    /// Get the CPU handle for an offset
    D3D12_GPU_DESCRIPTOR_HANDLE GPU(uint32_t offset = 0) const {
        return {gpu.ptr + offset * stride};
    }

    /// Advance this allocation for a sub-view
    /// \param offset offset to advance by
    /// \return new view
    ShaderExportOwnedHeapAllocation Advance(uint32_t offset) const {
        return {
            .heap = heap,
            .stride = stride,
            .cpu = CPU(offset),
            .gpu = GPU(offset)
        };
    }

    /// Owning heap
    ID3D12DescriptorHeap* heap{nullptr};

    /// Physical stride of each descriptor
    uint32_t stride{0};

    /// Heap handles
    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
};

struct ShaderExportOwnedHeapAllocator {
    /// Allocate a new set of descriptors
    /// \param deviceState owning device
    /// \param length number of descriptors to allocate
    /// \return allocation
    ShaderExportOwnedHeapAllocation Allocate(DeviceState* deviceState, uint32_t length);

    /// All live segments
    std::vector<ShaderExportOwnedHeapSegment> segments;
};
