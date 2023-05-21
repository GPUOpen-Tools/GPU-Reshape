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
class ShaderExportDescriptorAllocator;
struct FenceState;
struct PipelineState;
class DescriptorDataAppendAllocator;
struct DescriptorHeapState;

/// Tracked descriptor allocation
struct ShaderExportSegmentDescriptorAllocation {
    /// Owning allocation
    ShaderExportDescriptorAllocator* allocator{nullptr};

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
};

/// Single stream state
struct ShaderExportStreamState {
    ShaderExportStreamState(const Allocators& allocators) : segmentDescriptors(allocators) {
        
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

    /// Currently bound pipeline
    const PipelineState* pipeline{nullptr};

    /// Currently instrumented pipeline
    ID3D12PipelineState* pipelineObject{nullptr};

    /// Is the current pipeline instrumented?
    bool isInstrumented{false};

    /// All segment descriptors, lifetime bound to deferred segment
    Vector<ShaderExportSegmentDescriptorAllocation> segmentDescriptors;

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
        descriptorDataSegments(allocators),
        constantShaderDataBuffers(allocators),
        constantAllocator(allocators),
        commandContextHandles(allocators) {
        
    }
    
    /// Allocation for this segment
    ShaderExportSegmentInfo* allocation{nullptr};

    /// The patch command list, optional
    ImmediateCommandList immediatePatch;

    /// Patch descriptors
    ShaderExportSegmentDescriptorInfo patchDeviceCPUDescriptor;
    ShaderExportSegmentDescriptorInfo patchDeviceGPUDescriptor;

    /// Combined segment descriptors, lifetime bound to this segment
    Vector<ShaderExportSegmentDescriptorAllocation> segmentDescriptors;

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
