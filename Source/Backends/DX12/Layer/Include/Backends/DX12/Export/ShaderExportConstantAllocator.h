// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include <Backends/DX12/Allocation/Allocation.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>

// Forward declarations
class DeviceAllocator;

struct ShaderExportConstantAllocation {
    /// Underlying resource
    ID3D12Resource* resource{nullptr};

    /// Staging memory
    void* staging{nullptr};

    /// Offset into resource
    uint64_t offset{0};
};

struct ShaderExportConstantSegment {
    /// Check if this staging buffer can accommodate for a given length
    /// \param length given length
    /// \return true if this segment can accommodate
    bool CanAccomodate(size_t length) {
        return head + length <= size;
    }

    /// Underlying allocation
    Allocation allocation;

    /// Staging memory
    void* staging{nullptr};

    /// Total size of this allocation
    size_t size{0};

    /// Head offset of this allocation
    size_t head{0};
};

struct ShaderExportConstantAllocator {
    /// Allocate from this constant allocator
    /// \param deviceAllocator device allocator to be used
    /// \param length length of the allocation
    /// \return given allocation
    ShaderExportConstantAllocation Allocate(const ComRef<DeviceAllocator>& deviceAllocator, size_t length);

    /// All staging buffers
    std::vector<ShaderExportConstantSegment> staging;
};
