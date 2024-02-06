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
#include <Backends/Vulkan/States/DeviceMemoryRange.h>
#include <Backends/Vulkan/Vulkan.h>

// Common
#include <Common/Allocator/BTree.h>

// Std
#include <cstdint>
#include <vector>

// Forward declarations
struct BufferState;
struct ImageState;

enum class DeviceMemoryResourceType {
    None,
    Buffer,
    Image
};

struct DeviceMemoryResource {
    /// Type of the resource
    DeviceMemoryResourceType type{DeviceMemoryResourceType::None};

    /// Resource payload
    union {
        void* opaque{nullptr};
        BufferState* buffer;
        ImageState* image;
    };

    static DeviceMemoryResource Buffer(BufferState* state) {
        DeviceMemoryResource resource;
        resource.type = DeviceMemoryResourceType::Buffer;
        resource.buffer = state;
        return resource;
    }

    static DeviceMemoryResource Image(ImageState* state) {
        DeviceMemoryResource resource;
        resource.type = DeviceMemoryResourceType::Image;
        resource.image = state;
        return resource;
    }
};

struct DeviceMemoryEntry {
    /// Base offset for allocated range
    uint64_t baseOffset = 0ull;
    
    /// All resources for this entry
    std::vector<DeviceMemoryResource> resources;
};

struct DeviceMemoryRange {
    DeviceMemoryRange() : entries(Allocators{}) {
        
    }
    
    /// All virtual addresses tracked
    BTreeMap<uint64_t, DeviceMemoryEntry> entries;
};
