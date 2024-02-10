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
#include <Backends/DX12/Controllers/Versioning.h>
#include <Backends/DX12/ShaderData/ConstantShaderDataBuffer.h>

// Backend
#include <Backend/CommandContextHandle.h>

// Common
#include <Common/Containers/BucketPoolAllocator.h>
#include <Common/Containers/LinearBlockAllocator.h>

// Std
#include <vector>
#include <Backends/DX12/Resource/DescriptorDataSegment.h>

// Forward declarations
struct RootSignatureState;
struct ShaderExportSegmentInfo;
struct IncrementalFence;
class ShaderExportFixedTwoSidedDescriptorAllocator;
struct FenceState;
struct PipelineState;
class DescriptorDataAppendAllocator;
struct DescriptorHeapState;

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

/// Single stream state
struct ShaderExportStreamState {
    ShaderExportStreamState(const Allocators& allocators) : segmentDescriptors(allocators), referencedHeaps(allocators) {
        
    }

    /// Is this state pending?
    bool pending{false};
    
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

    /// Currently bound pipeline
    const PipelineState* pipeline{nullptr};

    /// Currently instrumented pipeline
    ID3D12PipelineState* pipelineObject{nullptr};

    /// Is the current pipeline instrumented?
    bool isInstrumented{false};

    /// All segment descriptors, lifetime bound to deferred segment
    Vector<ShaderExportSegmentDescriptorAllocation> segmentDescriptors;

    /// All references heaps
    Vector<DescriptorHeapState*> referencedHeaps;

    /// Shared constants buffer
    ConstantShaderDataBuffer constantShaderDataBuffer;

    /// Shared constants allocator
    ShaderExportConstantAllocator constantAllocator;

    /// Top level context handle
    CommandContextHandle commandContextHandle{kInvalidCommandContextHandle};
};

/// Single stream segment, i.e. submission
struct ShaderExportStreamSegment {
    ShaderExportStreamSegment(const Allocators& allocators) :
        segmentDescriptors(allocators),
        referencedHeaps(allocators),
        descriptorDataSegments(allocators),
        constantShaderDataBuffers(allocators),
        constantAllocator(allocators),
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

    /// Combined segment descriptors, lifetime bound to this segment
    Vector<ShaderExportSegmentDescriptorAllocation> segmentDescriptors;

    /// All references heaps
    Vector<DescriptorHeapState*> referencedHeaps;

    /// Combined descriptor data segments, lifetime bound to this segment
    Vector<DescriptorDataSegment> descriptorDataSegments;

    /// Combined context handles
    Vector<ConstantShaderDataBuffer> constantShaderDataBuffers;

    /// Combined constant allocators
    Vector<ShaderExportConstantAllocator> constantAllocator;

    /// Combined context handles
    Vector<CommandContextHandle> commandContextHandles;

    /// The next fence commit id to be waited for
    uint64_t fenceNextCommitId{UINT64_MAX};

    /// Synchronization fence (optional)
    IncrementalFence* fence{nullptr};

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
