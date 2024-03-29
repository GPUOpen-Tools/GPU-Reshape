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

// Common
#include <Common/Containers/ReferenceObject.h>

// Backend
#include <Backend/ShaderExportTypeInfo.h>

// Layer
#include <Backends/Vulkan/Allocation/MirrorAllocation.h>

// Std
#include <vector>

/// A single allocation allocation
struct ShaderExportStreamInfo {
    /// Type info of the originating message stream
    ShaderExportTypeInfo typeInfo;

    /// Descriptor object
    VkBuffer buffer{VK_NULL_HANDLE};

    /// View
    VkBufferView view{VK_NULL_HANDLE};

    /// Data allocation
    MirrorAllocation allocation;

    /// Actual byte size of the buffer (not allocation)
    uint64_t byteSize{0};
};

/// A batch of counters (for each stream), used for a single allocation
struct ShaderExportSegmentCounterInfo {
    /// Descriptor objects
    VkBuffer buffer{VK_NULL_HANDLE};
    VkBuffer bufferHost{VK_NULL_HANDLE};

    /// View
    VkBufferView view{VK_NULL_HANDLE};

    /// Counter allocation
    MirrorAllocation allocation;
};

/// A single allocation, partitioning is up to the allocation modes
struct ShaderExportSegmentInfo {
    /// Stream container, will reach stable size after a set submissions
    std::vector<ShaderExportStreamInfo> streams;

    /// Counter batch
    ShaderExportSegmentCounterInfo counter;

    /// Does this segment require initialization?
    bool pendingInitialization{true};
};
