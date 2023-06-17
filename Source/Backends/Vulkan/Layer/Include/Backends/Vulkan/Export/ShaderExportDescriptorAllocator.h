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

#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/Allocation/Allocation.h>
#include <Backends/Vulkan/Export/DescriptorInfo.h>
#include <Backends/Vulkan/States/PipelineLayoutBindingInfo.h>

// Backend
#include <Backend/ShaderData/ShaderDataInfo.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
struct DeviceDispatchTable;
struct ShaderExportSegmentInfo;
class DeviceAllocator;

class ShaderExportDescriptorAllocator : public TComponent<ShaderExportDescriptorAllocator> {
public:
    COMPONENT(ShaderExportDescriptorAllocator);

    ShaderExportDescriptorAllocator(DeviceDispatchTable* table);
    ~ShaderExportDescriptorAllocator();

    /// Install this allocator
    /// \return success state
    bool Install();

    /// Alloate a new segment
    /// \return the descriptor info
    ShaderExportSegmentDescriptorInfo Allocate();

    /// Update a segment
    /// \param info the descriptor segment to be updated
    /// \param descriptorChunk the descriptor chunk
    /// \param constantChunk the constant chunk
    void UpdateImmutable(const ShaderExportSegmentDescriptorInfo& info, VkBuffer descriptorChunk, VkBuffer constantsChunk);

    /// Update a segment
    /// \param info the descriptor segment to be updated
    /// \param segment the allocation segment [info] is bound to
    void Update(const ShaderExportSegmentDescriptorInfo& info, const ShaderExportSegmentInfo *segment);

    /// Free a given segment
    /// \param info
    void Free(const ShaderExportSegmentDescriptorInfo& info);

    /// Get the universal descriptor layout
    VkDescriptorSetLayout GetLayout() const {
        return layout;
    }

    /// Get the shared binding info
    PipelineLayoutBindingInfo GetBindingInfo() const {
        return bindingInfo;
    }

private:
    /// Create all dummy buffers
    void CreateDummyBuffer();

    /// Create the shared binding info
    void CreateBindingLayout();

private:
    struct PoolInfo {
        uint32_t index{0};
        uint32_t freeSets{0};
        VkDescriptorPool pool{VK_NULL_HANDLE};
    };

    /// Find an existing descriptor pool or allocate a new one, which may accomodate for a new set
    /// \return the pool
    PoolInfo& FindOrAllocatePool();

    /// All pools
    std::vector<PoolInfo> pools;

private:
    /// Export record layout
    VkDescriptorSetLayout layout{nullptr};

    /// Shared binding info
    PipelineLayoutBindingInfo bindingInfo;

    /// Dummy buffer
    VkBuffer dummyBuffer;
    VkBufferView dummyBufferView;
    Allocation dummyAllocation;

    /// Max sets per allocated pool
    uint32_t setsPerPool{64};

    /// The indexed bound for shader exports
    uint32_t exportBound{0};

    /// The indexed bound for data resources
    std::vector<ShaderDataInfo> dataResources;

private:
    DeviceDispatchTable* table;

    /// Shared lock
    std::mutex mutex;

    /// Components
    ComRef<DeviceAllocator> deviceAllocator;
};
