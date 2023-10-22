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

// Vulkan
#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan_core.h>

// Std
#include <atomic>

// TODO: Exactly what is the right way of registering new extension types?
static constexpr VkStructureType VK_STRUCTURE_TYPE_GPUOPEN_GPURESHAPE_CREATE_INFO = static_cast<VkStructureType>(1100000001);

// Layer name
#define VK_GPUOPEN_GPURESHAPE_LAYER_NAME "VK_LAYER_GPUOPEN_GRS"

// Forward declarations
class Registry;

/// Optional creation info for GPU Reshape
struct VkGPUOpenGPUReshapeCreateInfo {
    /// Structure type, must be VK_STRUCTURE_TYPE_GPUOPEN_GPURESHAPE_CREATE_INFO
    VkStructureType sType;

    /// Next extension
    const void *pNext;

    /// Shared registry
    Registry* registry;
};

/// Process state
struct VulkanGPUReshapeProcessState {
    /// Device UID allocator
    std::atomic<uint32_t> deviceUID; 
};

/// Shared process info
extern VulkanGPUReshapeProcessState VulkanGPUReshapeProcessInfo;
