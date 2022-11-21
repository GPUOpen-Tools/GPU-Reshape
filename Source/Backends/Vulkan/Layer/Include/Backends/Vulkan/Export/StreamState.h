#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include <Backends/Vulkan/States/PipelineState.h>
#include <Backends/Vulkan/Export/DescriptorInfo.h>

// Common
#include <Common/Containers/BucketPoolAllocator.h>

// Std
#include <vector>

// Forward declarations
struct ShaderExportSegmentInfo;
struct FenceState;
struct DescriptorDataAppendAllocator;

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

    /// The instrumentation overwrite mask
    uint32_t deviceDescriptorOverwriteMask{0x0};
};

/// Single stream state
struct ShaderExportStreamState {
    /// All bind points
    ShaderExportPipelineBindState pipelineBindPoints[static_cast<uint32_t>(PipelineType::Count)];

    /// The descriptor info, may not be mapped
    ShaderExportSegmentDescriptorInfo segmentDescriptorInfo;
};

/// Single stream segment, i.e. submission
struct ShaderExportStreamSegment {
    /// Allocation for this segment
    ShaderExportSegmentInfo* allocation{nullptr};

    /// Shared fence for this segment
    FenceState* fence{nullptr};

    /// The patch command buffer, optional
    VkCommandBuffer patchCommandBuffer{VK_NULL_HANDLE};

    /// The next fence commit id to be waited for
    uint64_t fenceNextCommitId{0};
};

/// The queue state
struct ShaderExportQueueState {
    VkQueue queue{VK_NULL_HANDLE};

    /// All submitted segments
    std::vector<ShaderExportStreamSegment*> liveSegments;
};
