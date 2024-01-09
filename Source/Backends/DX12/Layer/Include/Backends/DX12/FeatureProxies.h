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

// Backend
#include <Backend/FeatureHook.h>
#include <Backend/FeatureHookTable.h>

// Forward declarations
struct CommandListState;

struct FeatureHook_DrawInstanced : TFeatureHook<Hooks::DrawInstanced> {
    void operator()(CommandListState *state, CommandContext *context, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const {
        hook.Invoke(context, vertexCount, instanceCount, firstVertex, firstInstance);
    }
};

struct FeatureHook_DrawIndexedInstanced : TFeatureHook<Hooks::DrawIndexedInstanced> {
    void operator()(CommandListState *state, CommandContext *context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const {
        hook.Invoke(context, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
};

struct FeatureHook_Dispatch : TFeatureHook<Hooks::Dispatch> {
    void operator()(CommandListState *state, CommandContext *context, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) const {
        hook.Invoke(context, threadGroupX, threadGroupY, threadGroupZ);
    }
};

struct FeatureHook_CopyBufferRegion : TFeatureHook<Hooks::CopyResource> {
    void operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pDstBuffer, UINT64 DstOffset, ID3D12Resource *pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes) const;
};

struct FeatureHook_CopyTextureRegion : TFeatureHook<Hooks::CopyResource> {
    void operator()(CommandListState *object, CommandContext *context, const D3D12_TEXTURE_COPY_LOCATION* pDst, UINT DstX, UINT DstY, UINT DstZ, const D3D12_TEXTURE_COPY_LOCATION* pSrc, const D3D12_BOX* pSrcBox) const;
};

struct FeatureHook_CopyResource : TFeatureHook<Hooks::CopyResource> {
    void operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pDstResource, ID3D12Resource *pSrcResource) const;
};

struct FeatureHook_ResolveSubresource : TFeatureHook<Hooks::ResolveResource> {
    void operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pDstResource, UINT DstSubresource, ID3D12Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format) const;
};

struct FeatureHook_ClearDepthStencilView : TFeatureHook<Hooks::ClearResource> {
    void operator()(CommandListState *object, CommandContext *context, D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView, D3D12_CLEAR_FLAGS ClearFlags, FLOAT Depth, UINT8 Stencil, UINT NumRects, const D3D12_RECT *pRects) const;
};

struct FeatureHook_ClearRenderTargetView : TFeatureHook<Hooks::ClearResource> {
    void operator()(CommandListState *object, CommandContext *context, D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, const FLOAT ColorRGBA[4], UINT NumRects, const D3D12_RECT *pRects) const;
};

struct FeatureHook_ClearUnorderedAccessViewUint : TFeatureHook<Hooks::ClearResource> {
    void operator()(CommandListState *object, CommandContext *context, D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap, D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle, ID3D12Resource *pResource, const UINT Values[4], UINT NumRects, const D3D12_RECT *pRects) const;
};

struct FeatureHook_ClearUnorderedAccessViewFloat : TFeatureHook<Hooks::ClearResource> {
    void operator()(CommandListState *object, CommandContext *context, D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap, D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle, ID3D12Resource *pResource, const FLOAT Values[4], UINT NumRects, const D3D12_RECT *pRects) const;
};

struct FeatureHook_ResolveSubresourceRegion : TFeatureHook<Hooks::ResolveResource> {
    void operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, ID3D12Resource *pSrcResource, UINT SrcSubresource, D3D12_RECT *pSrcRect, DXGI_FORMAT Format, D3D12_RESOLVE_MODE ResolveMode) const;
};

struct FeatureHook_BeginRenderPass : TFeatureHook<Hooks::BeginRenderPass> {
    void operator()(CommandListState *object, CommandContext *context, UINT NumRenderTargets, const D3D12_RENDER_PASS_RENDER_TARGET_DESC *pRenderTargets, const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC *pDepthStencil, D3D12_RENDER_PASS_FLAGS Flags) const;
};

struct FeatureHook_EndRenderPass : TFeatureHook<Hooks::EndRenderPass> {
    void operator()(CommandListState *object, CommandContext *context) const;
};

struct FeatureHook_OMSetRenderTargets : TFeatureHook<Hooks::BeginRenderPass> {
    void operator()(CommandListState *object, CommandContext *context, UINT NumRenderTargetDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors, BOOL RTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor) const;
};
