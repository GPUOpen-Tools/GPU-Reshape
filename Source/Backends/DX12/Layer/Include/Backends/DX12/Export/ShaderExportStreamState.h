#pragma once

// Layer
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/ImmediateCommandList.h>
#include <Backends/DX12/Export/ShaderExportDescriptorInfo.h>

// Common
#include <Common/Containers/BucketPoolAllocator.h>

// Std
#include <vector>
#include <Backends/DX12/Resource/DescriptorDataSegment.h>

// Forward declarations
struct RootSignatureState;
struct ShaderExportSegmentInfo;
struct IncrementalFence;
struct ShaderExportDescriptorAllocator;
struct FenceState;
struct PipelineState;
struct DescriptorDataAppendAllocator;
struct DescriptorHeapState;

/// Tracked descriptor allocation
struct ShaderExportSegmentDescriptorAllocation {
    /// Owning allocation
    ShaderExportDescriptorAllocator* allocator{nullptr};

    /// Allocated info
    ShaderExportSegmentDescriptorInfo info;
};

/// Single stream state
struct ShaderExportStreamState {
    /// Currently bound root signature
    const RootSignatureState* rootSignature{nullptr};

    /// Currently bound pipeline
    const PipelineState* pipeline{nullptr};

    /// Currently bound (SRV, UAV, CBV) heap
    const DescriptorHeapState* heap{nullptr};

    /// Is the current pipeline instrumented?
    bool isInstrumented{false};

    /// Current mask of bound segments
    PipelineTypeSet pipelineSegmentMask{0};

    /// The descriptor info, may not be mapped
    ShaderExportSegmentDescriptorInfo currentSegment{};

    /// Descriptor data allocator tied to this segment
    DescriptorDataAppendAllocator* descriptorDataAllocator[static_cast<uint32_t>(PipelineType::Count)];

    /// All segment descriptors, lifetime bound to deferred segment
    std::vector<ShaderExportSegmentDescriptorAllocation> segmentDescriptors;
};

/// Single stream segment, i.e. submission
struct ShaderExportStreamSegment {
    /// Allocation for this segment
    ShaderExportSegmentInfo* allocation{nullptr};

    /// The patch command list, optional
    ImmediateCommandList immediatePatch;

    /// Patch descriptors
    ShaderExportSegmentDescriptorInfo patchDeviceCPUDescriptor;
    ShaderExportSegmentDescriptorInfo patchDeviceGPUDescriptor;

    /// Combined segment descriptors, lifetime bound to this segment
    std::vector<ShaderExportSegmentDescriptorAllocation> segmentDescriptors;

    /// Combined descriptor data segments, lifetime bound to this segment
    std::vector<DescriptorDataSegment> descriptorDataSegments;

    /// The next fence commit id to be waited for
    uint64_t fenceNextCommitId{UINT64_MAX};

    /// Synchronization fence (optional)
    IncrementalFence* fence{nullptr};
};

/// The queue state
struct ShaderExportQueueState {
    ID3D12CommandQueue* queue{nullptr};

    /// All submitted segments
    std::vector<ShaderExportStreamSegment*> liveSegments;
};
