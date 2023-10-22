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

// General compile time toggles
#include "Config.h"

// Common
#include <Common/Alloca.h>
#include <Common/Allocators.h>
#include <Common/Assert.h>

// System
#include <AgilitySDK/d3d12.h>
#include <dxgi1_6.h>
#include <dxgi.h>

// Cleanup
#undef OPAQUE
#undef min
#undef max

/// Unwrapping UUID
///  ! May be replaced by faster methods, such as allocation arena checking
/// {D3CD71B6-5E41-4A9C-BB04-7D8EF27CFB57}
static const GUID IID_Unwrap = { 0xd3cd71b6, 0x5e41, 0x4a9c, { 0xbb, 0x4, 0x7d, 0x8e, 0xf2, 0x7c, 0xfb, 0x57 } };

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
