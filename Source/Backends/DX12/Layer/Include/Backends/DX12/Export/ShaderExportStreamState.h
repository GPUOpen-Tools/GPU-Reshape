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
#include <Backends/DX12/DX12.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/ImmediateCommandList.h>
#include <Backends/DX12/Export/ShaderExportDescriptorInfo.h>
#include <Backends/DX12/Export/ShaderExportConstantAllocator.h>
#include <Backends/DX12/Export/ShaderExportDeviceAllocator.h>
#include <Backends/DX12/Controllers/Versioning.h>
#include <Backends/DX12/ShaderData/ConstantShaderDataBuffer.h>
#include <Backends/DX12/Export/ShaderExportOwnedHeapAllocator.h>

// Backend
#include <Backend/CommandContextHandle.h>
#include <Backend/CommandContext.h>
#include <Backend/IL/Execution/ExecutionInfo.h>

// Common
#include <Common/Containers/LinearBlockAllocator.h>

// Std
#include <vector>

// Forward declarations
struct RootSignatureState;
struct ShaderExportSegmentInfo;
struct ShaderExportStreamStateRaytracingCache;
struct ShaderExportStreamStateBarrierTracking;
struct VirtualAddressMappingTablePersistentVersion;
struct IncrementalFence;
class ShaderExportFixedTwoSidedDescriptorAllocator;
struct FenceState;
struct PipelineState;
class DescriptorDataAppendAllocator;
struct DescriptorHeapState;
struct ShaderExportStreamSegment;

/// Tracked descriptor allocation
struct ShaderExportSegmentDescriptorAllocation {
    /// Owning allocation
    ShaderExportFixedTwoSidedDescriptorAllocator* allocator{nullptr};

    /// Allocated info
    ShaderExportSegmentDescriptorInfo info;
};

struct ShaderExportRootConstantParameterValue {
    /// Bound constant data
    void* data;

    /// Bytes of mapped constant data
    uint32_t dataByteCount;
};

enum class ShaderExportRootParameterValueType {
    None,
    Descriptor,
    SRV,
    UAV,
    CBV,
    Constant
};

union ShaderExportRootParameterValuePayload {
    /// Valid for ShaderExportRootParameterValueType::Descriptor
    D3D12_GPU_DESCRIPTOR_HANDLE descriptor;

    /// Valid for ShaderExportRootParameterValueType::SRV, UAV, CBV
    D3D12_GPU_VIRTUAL_ADDRESS virtualAddress;

    /// Valid for ShaderExportRootParameterValueType::Constant
    ShaderExportRootConstantParameterValue constant;
};

struct ShaderExportRootParameterValue {
    /// Create an invalid parameter
    static ShaderExportRootParameterValue None() {
        return {
            .type = ShaderExportRootParameterValueType::None
        };
    }

    /// Create a descriptor parameter
    static ShaderExportRootParameterValue Descriptor(D3D12_GPU_DESCRIPTOR_HANDLE descriptor) {
        return {
            .type = ShaderExportRootParameterValueType::Descriptor,
            .payload = ShaderExportRootParameterValuePayload {
                .descriptor = descriptor
            }
        };
    }

    /// Create a virutal address parameter
    static ShaderExportRootParameterValue VirtualAddress(ShaderExportRootParameterValueType type, D3D12_GPU_VIRTUAL_ADDRESS address) {
        return {
            .type = type,
            .payload = ShaderExportRootParameterValuePayload {
                .virtualAddress = address
            }
        };
    }

    /// Parameter type
    ShaderExportRootParameterValueType type{ShaderExportRootParameterValueType::None};

    /// Parameter data
    ShaderExportRootParameterValuePayload payload;
};

struct ShaderExportStreamBindState {
    /// Currently bound root signature
    const RootSignatureState* rootSignature{nullptr};

    /// Descriptor data allocator tied to this segment
    DescriptorDataAppendAllocator* descriptorDataAllocator{nullptr};

    /// On-demand allocator for root data
    LinearBlockAllocator<1024> rootConstantAllocator;

    /// All currently bound root data
    ShaderExportRootParameterValue persistentRootParameters[MaxRootSignatureDWord];

#ifndef NDEBUG
    /// Validation binding mask
    uint64_t bindMask{0};
#endif // NDEBUG
};

struct ShaderExportRenderPassState {
    /// Number of render passes bound
    uint32_t renderTargetCount{0};

    /// All render pass data
    D3D12_RENDER_PASS_RENDER_TARGET_DESC renderTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];

    /// Optional depth stencil data
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencil;

    /// All flags
    D3D12_RENDER_PASS_FLAGS flags;
    
    /// Are we inside a render pass
    bool insideRenderPass{false};
};

struct ShaderExportSegmentDescriptorEntry {
    /// The heaps that was referenced
    DescriptorHeapState* resourceHeap{nullptr};
    DescriptorHeapState* samplerHeap{nullptr};

    /// The allocated segment
    ShaderExportSegmentDescriptorAllocation segment{};
};

#ifndef NDEBUG
struct ShaderExportStreamStateDebugStream {
    /// Identifying name
    std::string name;

    /// Resource to be mapped
    ID3D12Resource* resource{nullptr};

    /// Offset to map from
    uint64_t offset{0};

    /// Number of bytes to map from the offset
    uint64_t length{0};
};
#endif // NDEBUG

struct ShaderExportStreamStateBackendMessages {
    /// Optional, allocated if the application requires patching that needs feedback
    Allocation allocation;

    /// If true, requires initialization before use
    bool pendingInitialization = true;
};

struct ShaderExportStreamViewportState {
    /// Currently set viewport
    D3D12_VIEWPORT state{};

    /// Assigned uid
    uint32_t rollingUID{0};
};

struct ShaderExportStreamMarkerEntryState {
    /// Is this hierarhical?
    bool hierarchical = false;

    /// CRC32 hash
    uint32_t hash32{0};
};

struct ShaderExportStreamMarkerState {
    /// All markers
    TrivialStackVector<ShaderExportStreamMarkerEntryState, kMaxExecutionInfoMarkerCount> stack;
};

struct ShaderExportStreamPersistentState {
    /// All host descriptor handles
    TrivialStackVector<D3D12_CPU_DESCRIPTOR_HANDLE, 4u> vamtVersionDescriptorHandles;
};

/// Single stream state
struct ShaderExportStreamState {
    ShaderExportStreamState(const Allocators& allocators) : segmentDescriptors(allocators), referencedHeaps(allocators) {
        
    }

    /// Is this state pending?
    bool pending{false};

    /// Does this state have any descriptor data?
    bool hasDescriptorState{false};
    
    /// Currently bound heaps
    DescriptorHeapState* resourceHeap{nullptr};
    DescriptorHeapState* samplerHeap{nullptr};

    /// Current mask of bound segments
    PipelineTypeSet pipelineSegmentMask{0};

    /// The descriptor info, may not be mapped
    ShaderExportSegmentDescriptorInfo currentSegment{};

    /// Bind states
    ShaderExportStreamBindState bindStates[static_cast<uint32_t>(PipelineType::Count)];

    /// Graphics render pass
    ShaderExportRenderPassState renderPass;

    /// Currently set viewport
    ShaderExportStreamViewportState viewport;

    /// Currently set markers
    ShaderExportStreamMarkerState markers;
    
    /// Currently bound pipeline
    const PipelineState* pipeline{nullptr};

    /// Currently instrumented pipeline
    IUnknown* pipelineObject{nullptr};

    /// Current pipeline instrument, null if not instrumented
    PipelineInstrument* pipelineInstrument{nullptr};

    /// All segment descriptors, lifetime bound to deferred segment
    Vector<ShaderExportSegmentDescriptorEntry> segmentDescriptors;

    /// All references heaps
    Vector<DescriptorHeapState*> referencedHeaps;

    /// Shared constants buffer
    ConstantShaderDataBuffer constantShaderDataBuffer;

    /// Shared constants allocator
    /// Useful for small transient allocations
    ShaderExportConstantAllocator constantAllocator;

    /// Shared device allocator
    /// Useful for larger, stateful allocations
    ShaderExportDeviceAllocator deviceAllocator;

    /// Shared heap allocator
    /// Useful for larger descriptor allocations that shouldnt pollute the small reserved region
    /// Does however require user heap reconstruction when used
    ShaderExportOwnedHeapAllocator heapAllocator;

    /// Per-state barrier tracking for cache invalidation, allocated on demand
    ShaderExportStreamStateBarrierTracking* barrierTracking{nullptr};

    /// Per-state caches, allocated on demand
    ShaderExportStreamStateRaytracingCache* raytracingCache{nullptr};

    /// All backend messages
    ShaderExportStreamStateBackendMessages backendMessages;
    
    /// All persistent state
    ShaderExportStreamPersistentState persistentState;

    /// Top level context handle
    CommandContextHandle commandContextHandle{kInvalidCommandContextHandle};

    /// The segment that currently references this state
    ShaderExportStreamSegment* segment{nullptr};

#ifndef NDEBUG
    /// All pending debug streams
    std::vector<ShaderExportStreamStateDebugStream> debugStreams;
#endif // NDEBUG
};

struct ShaderExportStreamSegmentUserContext {
    /// Segment context
    CommandContext commandContext;

    /// Streaming state
    ShaderExportStreamState* streamState{nullptr};
};

/// Single stream segment, i.e. submission
struct ShaderExportStreamSegment {
    ShaderExportStreamSegment(const Allocators& allocators) :
        referencedHeaps(allocators),
        commandContextHandles(allocators) {
        
    }
    
    /// Allocation for this segment
    ShaderExportSegmentInfo* allocation{nullptr};

    /// The patch command lists, optional
    ImmediateCommandList immediatePrePatch;
    ImmediateCommandList immediatePostPatch;

    /// Patch descriptors
    ShaderExportSegmentDescriptorInfo patchDeviceCPUDescriptor;
    ShaderExportSegmentDescriptorInfo patchDeviceGPUDescriptor;

    /// All references heaps
    Vector<DescriptorHeapState*> referencedHeaps;

    /// Combined context handles
    Vector<CommandContextHandle> commandContextHandles;

    /// Optional contexts for user command buffers
    ShaderExportStreamSegmentUserContext userPreContext;
    ShaderExportStreamSegmentUserContext userPostContext;

    /// All referenced streaming states
    std::vector<ShaderExportStreamState*> streamStates;

    /// The next fence commit id to be waited for
    uint64_t fenceNextCommitId{UINT64_MAX};

    /// Synchronization fence (optional)
    IncrementalFence* fence{nullptr};
    
    /// Persistent versions
    VirtualAddressMappingTablePersistentVersion* vamtPersistentVersion{nullptr};

    /// Segmentation point during submission
    VersionSegmentationPoint versionSegPoint{};
};

/// The queue state
struct ShaderExportQueueState {
    ShaderExportQueueState(const Allocators& allocators) : liveSegments(allocators) {
        
    }
    
    ID3D12CommandQueue* queue{nullptr};

    /// All submitted segments
    Vector<ShaderExportStreamSegment*> liveSegments;
};
