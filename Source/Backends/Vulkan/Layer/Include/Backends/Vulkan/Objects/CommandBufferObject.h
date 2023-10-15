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
#include "Backends/Vulkan/CommandBufferDispatchTable.Gen.h"
#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/States/PipelineType.h"

// Common
#include "Common/Containers/ReferenceObject.h"

// Backend
#include <Backend/CommandContext.h>

// Std
#include <vector>

/// Forward declarations
struct DeviceDispatchTable;
struct ShaderExportStreamState;
struct CommandPoolState;
struct PipelineState;

/// Wrapped command buffer object
struct CommandBufferObject {
    /// Add a referenced object to the GPU lifetime of this command buffer
    ///   ? Note, not immediate, lifetime completion is checked when queried
    /// \param obj the referenced object, once the gpu has exhausted the command buffer, the objects are released
    void AddLifetime(ReferenceObject* obj) {
        obj->AddUser();
        gpuReferences.emplace_back(obj);
    }

    /// Object dispatch
    void*                next_dispatchTable;
    VkCommandBuffer      object;
    DeviceDispatchTable* table;
    CommandPoolState*    pool;

    /// Immediate context
    struct
    {
        /// Currently bound pipeline, not subject to lifetime extensions due to spec requirements
        PipelineState* pipeline{nullptr};

#if TRACK_DESCRIPTOR_SETS
        VkDescriptorSet descriptorSets[static_cast<uint32_t>(PipelineType::Count)][512]{};
#endif
    } context;

    /// Acquired dispatch table
    CommandBufferDispatchTable dispatchTable;

    /// Current streaming state
    ShaderExportStreamState* streamState;

    /// User context
    CommandContext userContext;

    /// GPU lifetime references
    std::vector<ReferenceObject*> gpuReferences;
};
