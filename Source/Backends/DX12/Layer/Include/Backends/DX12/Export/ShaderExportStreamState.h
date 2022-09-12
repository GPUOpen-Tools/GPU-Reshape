#pragma once

// Layer
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/Export/ShaderExportDescriptorInfo.h>

// Common
#include <Common/Containers/BucketPoolAllocator.h>

// Std
#include <vector>

// Forward declarations
struct RootSignatureState;
struct ShaderExportSegmentInfo;
struct IncrementalFence;
struct FenceState;
struct PipelineState;

/// Single stream state
struct ShaderExportStreamState {
    /// Currently bound root signature
    const RootSignatureState* rootSignature{nullptr};

    /// Currently bound pipeline
    const PipelineState* pipeline{nullptr};

    /// Is the current pipeline instrumented?
    bool isInstrumented{false};

    /// Current mask of bound segments
    PipelineTypeSet pipelineSegmentMask{0};

    /// The descriptor info, may not be mapped
    ShaderExportSegmentDescriptorInfo currentSegment{};

    /// All segments
    std::vector<ShaderExportSegmentDescriptorInfo> segmentDescriptors;
};

/// Single stream segment, i.e. submission
struct ShaderExportStreamSegment {
    /// Allocation for this segment
    ShaderExportSegmentInfo* allocation{nullptr};

    /// The patch command list, optional
    ID3D12GraphicsCommandList* patchCommandList{nullptr};

    ShaderExportSegmentDescriptorInfo patchDeviceCPUDescriptor;
    ShaderExportSegmentDescriptorInfo patchDeviceGPUDescriptor;

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
