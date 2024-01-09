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
#include <Backends/Vulkan/States/FenceState.h>
#include <Backends/Vulkan/Resource/PhysicalResourceMappingTableQueueState.h>

// Common
#include <Common/Containers/ObjectPool.h>

// Std
#include <cstdint>
#include <vector>
#include <mutex>

// Forward declarations
struct ShaderExportQueueState;
struct DeviceDispatchTable;

struct QueueState {
    ~QueueState();

    VkCommandBuffer PopCommandBuffer();

    void PushCommandBuffer(VkCommandBuffer commandBuffer);

    /// Backwards reference
    DeviceDispatchTable* table;

    /// User pipeline
    VkQueue object{VK_NULL_HANDLE};

    /// Family index of this queue
    uint32_t familyIndex{~0U};

    /// Shared command pool
    VkCommandPool commandPool{VK_NULL_HANDLE};

    /// Immediate command buffers
    std::vector<VkCommandBuffer> commandBuffers;

    /// Object pools
    ObjectPool<FenceState> pools_fences;

    /// Current export state
    ShaderExportQueueState* exportState{nullptr};

    /// PRMT queue state
    PhysicalResourceMappingTableQueueState prmtState;

    /// Shared lock
    std::mutex mutex;

    /// Unique identifier, unique for the type
    uint64_t uid;
};
