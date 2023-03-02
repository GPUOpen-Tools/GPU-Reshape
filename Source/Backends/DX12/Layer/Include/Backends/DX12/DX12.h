#pragma once

// General compile time toggles
#include "Config.h"

// Common
#include <Common/Alloca.h>
#include <Common/Allocators.h>
#include <Common/Assert.h>

// System
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi.h>

// Cleanup
#undef OPAQUE
#undef min
#undef max

/// Get the vtable
template<typename T = void>
inline T* GetVTableRaw(void* object) {
    if (!object) {
        return nullptr;
    }

    return *(T **) object;
}

/// Get the vtable
template<typename T = void>
inline T *&GetVTableRawRef(void* object) {
    return *(T **) object;
}

/// Common allocation tags
static constexpr AllocationTag kAllocStateDevice = "DX12.State.Device"_AllocTag;
static constexpr AllocationTag kAllocStateRootSignature = "DX12.State.RootSignature"_AllocTag;
static constexpr AllocationTag kAllocStateCommandQueue = "DX12.State.CommandQueue"_AllocTag;
static constexpr AllocationTag kAllocStateIncrementalFence = "DX12.State.IncrementalFence"_AllocTag;
static constexpr AllocationTag kAllocStateCommandSignatureState = "DX12.State.CommandSignatureState"_AllocTag;
static constexpr AllocationTag kAllocStateCommandAllocator = "DX12.State.CommandAllocator"_AllocTag;
static constexpr AllocationTag kAllocStateCommandList = "DX12.State.CommandList"_AllocTag;
static constexpr AllocationTag kAllocStateDescriptorHeap = "DX12.State.DescriptorHeap"_AllocTag;
static constexpr AllocationTag kAllocStateDXGIFactory = "DX12.State.DXGIFactory"_AllocTag;
static constexpr AllocationTag kAllocStateFence = "DX12.State.Fence"_AllocTag;
static constexpr AllocationTag kAllocStateShader = "DX12.State.Shader"_AllocTag;
static constexpr AllocationTag kAllocStatePipeline = "DX12.State.Pipeline"_AllocTag;
static constexpr AllocationTag kAllocStatePipelineLibrary = "DX12.State.PipelineLibrary"_AllocTag;
static constexpr AllocationTag kAllocStateResource = "DX12.State.Resource"_AllocTag;
static constexpr AllocationTag kAllocStateSwapchain = "DX12.State.Swapchain"_AllocTag;
static constexpr AllocationTag kAllocShaderExport = "DX12.ShaderExport"_AllocTag;
static constexpr AllocationTag kAllocInstrumentation = "DX12.Instrumentation"_AllocTag;
static constexpr AllocationTag kAllocPRMT = "DX12.PRMT"_AllocTag;
static constexpr AllocationTag kAllocTracking = "DX12.Tracking"_AllocTag;
static constexpr AllocationTag kAllocRegistry = "DX12.Registry"_AllocTag;
static constexpr AllocationTag kAllocShaderData = "DX12.ShaderData"_AllocTag;
static constexpr AllocationTag kAllocShaderProgram = "DX12.ShaderProgram"_AllocTag;
static constexpr AllocationTag kAllocSGUID = "DX12.SGUID"_AllocTag;
