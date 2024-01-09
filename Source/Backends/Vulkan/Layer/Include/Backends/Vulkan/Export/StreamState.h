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
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/Export/DescriptorInfo.h>
#include <Backends/Vulkan/Resource/DescriptorDataSegment.h>
#include <Backends/Vulkan/Controllers/Versioning.h>
#include <Backends/Vulkan/ShaderData/ConstantShaderDataBuffer.h>

// Backend
#include <Backend/CommandContextHandle.h>

// Common
#include <Common/Containers/BucketPoolAllocator.h>

// Std
#include <vector>

// Forward declarations
struct ShaderExportSegmentInfo;
struct PushDescriptorSegment;
struct FenceState;
class DescriptorDataAppendAllocator;
class PushDescriptorAppendAllocator;
class PhysicalResourceMappingTablePersistentVersion;

/// Tracked descriptor allocation
struct ShaderExportSegmentDescriptorAllocation {
    /// The descriptor info, may not be mapped
    ShaderExportSegmentDescriptorInfo info;

    /// Current segment chunk, checked for rolling
    VkBuffer descriptorRollChunk{VK_NULL_HANDLE};
};

/// Descriptor state for re-binding
struct ShaderExportDescriptorState {
    /// All dynamic offsets
    BucketPoolAllocation<uint32_t> dynamicOffsets{};

    /// Source compatability hash
    uint64_t compatabilityHash;

    /// Set, lifetime bound to the command buffer
    VkDescriptorSet set;
};

/// Single bind state
struct ShaderExportPipelineBindState {
    /// Current descriptor sets
    std::vector<ShaderExportDescriptorState> persistentDescriptorState;

    /// Descriptor data allocator tied to the segment
    DescriptorDataAppendAllocator* descriptorDataAllocator{nullptr};

    /// Push state append allocator
    PushDescriptorAppendAllocator* pushDescriptorAppendAllocator{nullptr};

    /// Currently bound pipeline
    const PipelineState* pipeline{nullptr};

    /// Currently bound vk object
    VkPipeline pipelineObject{VK_NULL_HANDLE};

    /// Is the current pipeline instrumented?
    bool isInstrumented{false};

    /// The descriptor info, may not be mapped
    ShaderExportSegmentDescriptorAllocation currentSegment{};

    /// The instrumentation overwrite mask
    uint32_t deviceDescriptorOverwriteMask{0x0};
};

/// Render pass state
struct ShaderExportRenderPassState {
    /// Current deep copy
    VkRenderPassBeginInfoDeepCopy deepCopy;

    /// Current contents
    VkSubpassContents subpassContents;

    /// Are we inside a render pass? Also serves as validation for the deep copy
    bool insideRenderPass{false};
};

/// Single stream state
struct ShaderExportStreamState {
    /// Is this state pending?
    bool pending{false};
    
    /// All bind points
    ShaderExportPipelineBindState pipelineBindPoints[static_cast<uint32_t>(PipelineType::Count)];

    /// Graphics render pass
    ShaderExportRenderPassState renderPass;

    /// All segment descriptors, lifetime bound to deferred segment
    std::vector<ShaderExportSegmentDescriptorAllocation> segmentDescriptors;

    /// Current push constant data
    std::vector<uint8_t> persistentPushConstantData;

    /// Shared constants buffer
    ConstantShaderDataBuffer constantShaderDataBuffer;

    /// Top context handle
    CommandContextHandle commandContextHandle{kInvalidCommandContextHandle};
};

/// Single stream segment, i.e. submission
struct ShaderExportStreamSegment {
    /// Allocation for this segment
    ShaderExportSegmentInfo* allocation{nullptr};

    /// Shared fence for this segment
    FenceState* fence{nullptr};

    /// The patch command buffers, optional
    VkCommandBuffer prePatchCommandBuffer{VK_NULL_HANDLE};
    VkCommandBuffer postPatchCommandBuffer{VK_NULL_HANDLE};

    /// The next fence commit id to be waited for
    uint64_t fenceNextCommitId{0};

    /// Combined segment descriptors, lifetime bound to this segment
    std::vector<ShaderExportSegmentDescriptorAllocation> segmentDescriptors;

    /// Combined descriptor data segments, lifetime bound to this segment
    std::vector<DescriptorDataSegment> descriptorDataSegments;

    /// All pending push segments
    std::vector<PushDescriptorSegment> pushDescriptorSegments;

    /// Combined context handles
    std::vector<CommandContextHandle> commandContextHandles;

    /// Persistent version for the PRM-Table
    PhysicalResourceMappingTablePersistentVersion* prmtPersistentVersion{nullptr};

    /// Versioning segmentation point during submission
    VersionSegmentationPoint versionSegPoint{};
};

/// The queue state
struct ShaderExportQueueState {
    VkQueue queue{VK_NULL_HANDLE};

    /// All submitted segments
    std::vector<ShaderExportStreamSegment*> liveSegments;
};
