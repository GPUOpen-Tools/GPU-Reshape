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
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/States/DeviceMemoryRange.h>

// Std
#include <mutex>

// Forward declarations
struct DeviceDispatchTable;

struct DeviceMemoryState {
    /// Backwards reference
    DeviceDispatchTable* table;

    /// User memory
    VkDeviceMemory object{VK_NULL_HANDLE};

    /// Complete range for tracking
    DeviceMemoryRange range;

    /// Length of this memory
    size_t length{UINT64_MAX};

    /// Currently known mapped range
    /// This may change into a proper map in the future for better granularity
    uint64_t mappedOffsetStart{UINT64_MAX};
    uint64_t mappedOffsetEnd{0};

    /// Has this block been mapped before?
    bool hasMapped{false};

    /// Shared lock for this memory allocation
    /// Number of allocations are low enough so that this is not that costly
    std::mutex lock;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
