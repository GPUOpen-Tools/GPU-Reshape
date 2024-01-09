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

#include <Backends/DX12/FeatureProxies.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/States/ResourceState.h>
#include <Backends/DX12/Export/ShaderExportStreamState.h>
#include <Backends/DX12/Resource/PhysicalResourceMappingTable.h>
#include <Backends/DX12/Table.Gen.h>

// Backend
#include <Backend/Command/BufferDescriptor.h>
#include <Backend/Command/TextureDescriptor.h>
#include <Backend/Command/ResourceInfo.h>
#include <Backend/Command/RenderPassInfo.h>
#include <Backend/Command/AttachmentInfo.h>

/// Get a resource token
/// \param state object state
/// \return given token
template<typename T>
static ResourceToken GetResourceToken(const T* state) {
    return ResourceToken {
        .puid = state->virtualMapping.puid,
        .type = static_cast<Backend::IL::ResourceTokenType>(state->virtualMapping.type),
        .srb= state->virtualMapping.srb
    };
}

/// Get the resource state from a heap handle
/// \param state given command list state
/// \param handle opaque handle
/// \return state, may be nullptr
[[maybe_unused]]
static ResourceState* GetResourceStateFromHeapHandle(CommandListState* state, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    auto table = GetTable(state->parent);

    // Get heap from handle ptr
    DescriptorHeapState* heap = table.state->cpuHeapTable.Find(type, handle.ptr);

    // Get state
    return heap->GetStateFromHeapHandle(handle);
}

/// Get the resource token from a heap handle
/// \param state given command list state
/// \param handle opaque handle
/// \return resource token
static ResourceToken GetResourceTokenFromHeapHandle(CommandListState* state, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    auto table = GetTable(state->parent);

    // Get heap from handle ptr
    DescriptorHeapState* heap = table.state->cpuHeapTable.Find(type, handle.ptr);

    // Get virtual mapping
    VirtualResourceMapping virtualMapping = heap->GetVirtualMappingFromHeapHandle(handle);

    // To token
    return ResourceToken {
        .puid = virtualMapping.puid,
        .type = static_cast<Backend::IL::ResourceTokenType>(virtualMapping.type),
        .srb= virtualMapping.srb
    };
}

void FeatureHook_CopyBufferRegion::operator()(CommandListState *list, CommandContext *context, ID3D12Resource* pDstBuffer, UINT64 DstOffset, ID3D12Resource* pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes) const {
    // Get states
    ResourceState* dstState = GetState(pDstBuffer);
    ResourceState* srcState = GetState(pSrcBuffer);

    // Setup source descriptor
    BufferDescriptor srcDescriptor{
        .offset = SrcOffset,
        .width = NumBytes,
        .uid = 0u
    };

    // Setup destination descriptor
    BufferDescriptor dstDescriptor{
        .offset = DstOffset,
        .width = NumBytes,
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Buffer(GetResourceToken(srcState), &srcDescriptor),
        ResourceInfo::Buffer(GetResourceToken(dstState), &dstDescriptor)
    );
}

void FeatureHook_CopyTextureRegion::operator()(CommandListState *object, CommandContext *context, const D3D12_TEXTURE_COPY_LOCATION* pDst, UINT DstX, UINT DstY, UINT DstZ, const D3D12_TEXTURE_COPY_LOCATION* pSrc, const D3D12_BOX* pSrcBox) const {
    // Get states
    ResourceState* dstState = GetState(pDst->pResource);
    ResourceState* srcState = GetState(pSrc->pResource);

    // Setup source descriptor
    TextureDescriptor srcDescriptor{
        .region = TextureRegion { },
        .uid = 0u
    };

    // Setup destination descriptor
    TextureDescriptor dstDescriptor{
        .region = TextureRegion { },
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(GetResourceToken(srcState), &srcDescriptor),
        ResourceInfo::Texture(GetResourceToken(dstState), &dstDescriptor)
    );
}

void FeatureHook_CopyResource::operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pDstResource, ID3D12Resource *pSrcResource) const {
    // Get states
    ResourceState* dstState = GetState(pDstResource);
    ResourceState* srcState = GetState(pSrcResource);

    // Handle type
    switch (static_cast<Backend::IL::ResourceTokenType>(dstState->virtualMapping.type)) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case Backend::IL::ResourceTokenType::Texture: {
            // Setup source descriptor
            TextureDescriptor srcDescriptor{
                .region = TextureRegion { },
                .uid = 0u
            };

            // Setup destination descriptor
            TextureDescriptor dstDescriptor{
                .region = TextureRegion { },
                .uid = 0u
            };

            // Invoke hook
            hook.Invoke(
                context,
                ResourceInfo::Texture(GetResourceToken(srcState), &srcDescriptor),
                ResourceInfo::Texture(GetResourceToken(dstState), &dstDescriptor)
            );
            break;
        }
        case Backend::IL::ResourceTokenType::Buffer: {
            // Setup source descriptor
            BufferDescriptor srcDescriptor{
                .offset = 0u,
                .width = ~0u,
                .uid = 0u
            };

            // Setup destination descriptor
            BufferDescriptor dstDescriptor{
                .offset = 0u,
                .width = ~0u,
                .uid = 0u
            };

            // Invoke hook
            hook.Invoke(
                context,
                ResourceInfo::Buffer(GetResourceToken(srcState), &srcDescriptor),
                ResourceInfo::Buffer(GetResourceToken(dstState), &dstDescriptor)
            );
            break;
        }
    }
}

void FeatureHook_ResolveSubresource::operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pDstResource, UINT DstSubresource, ID3D12Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format) const {
    // Get states
    ResourceState* dstState = GetState(pDstResource);
    ResourceState* srcState = GetState(pSrcResource);

    // Setup source descriptor
    TextureDescriptor srcDescriptor{
        .region = TextureRegion { },
        .uid = 0u
    };

    // Setup destination descriptor
    TextureDescriptor dstDescriptor{
        .region = TextureRegion { },
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(GetResourceToken(srcState), &srcDescriptor),
        ResourceInfo::Texture(GetResourceToken(dstState), &dstDescriptor)
    );
}

void FeatureHook_ClearDepthStencilView::operator()(CommandListState *object, CommandContext *context, D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView, D3D12_CLEAR_FLAGS ClearFlags, FLOAT Depth, UINT8 Stencil, UINT NumRects, const D3D12_RECT *pRects) const {
    // Get states
    ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DepthStencilView);

    // Setup source descriptor
    TextureDescriptor descriptor{
        .region = TextureRegion { },
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(token, &descriptor)
    );
}

void FeatureHook_ClearRenderTargetView::operator()(CommandListState *object, CommandContext *context, D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, const FLOAT *ColorRGBA, UINT NumRects, const D3D12_RECT *pRects) const {
    // Get states
    ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RenderTargetView);

    // Setup source descriptor
    TextureDescriptor descriptor{
        .region = TextureRegion { },
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(token, &descriptor)
    );
}

void FeatureHook_ClearUnorderedAccessViewUint::operator()(CommandListState *object, CommandContext *context, D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap, D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle, ID3D12Resource *pResource, const UINT *Values, UINT NumRects, const D3D12_RECT *pRects) const {
    // Get states
    ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, ViewCPUHandle);

    // Setup source descriptor
    TextureDescriptor descriptor{
        .region = TextureRegion { },
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(token, &descriptor)
    );
}

void FeatureHook_ClearUnorderedAccessViewFloat::operator()(CommandListState *object, CommandContext *context, D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap, D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle, ID3D12Resource *pResource, const FLOAT *Values, UINT NumRects, const D3D12_RECT *pRects) const {
    // Get states
    ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, ViewCPUHandle);

    // Setup source descriptor
    TextureDescriptor descriptor{
        .region = TextureRegion { },
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(token, &descriptor)
    );
}

void FeatureHook_ResolveSubresourceRegion::operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, ID3D12Resource *pSrcResource, UINT SrcSubresource, D3D12_RECT *pSrcRect, DXGI_FORMAT Format, D3D12_RESOLVE_MODE ResolveMode) const {
    // Get states
    ResourceState* dstState = GetState(pDstResource);
    ResourceState* srcState = GetState(pSrcResource);

    // Setup source descriptor
    TextureDescriptor srcDescriptor{
        .region = TextureRegion { },
        .uid = 0u
    };

    // Setup destination descriptor
    TextureDescriptor dstDescriptor{
        .region = TextureRegion { },
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(GetResourceToken(srcState), &srcDescriptor),
        ResourceInfo::Texture(GetResourceToken(dstState), &dstDescriptor)
    );
}

void FeatureHook_BeginRenderPass::operator()(CommandListState *object, CommandContext *context, UINT NumRenderTargets, const D3D12_RENDER_PASS_RENDER_TARGET_DESC *pRenderTargets, const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC *pDepthStencil, D3D12_RENDER_PASS_FLAGS Flags) const {
    TextureDescriptor* descriptors = ALLOCA_ARRAY(TextureDescriptor, NumRenderTargets);
    AttachmentInfo*    attachments = ALLOCA_ARRAY(AttachmentInfo, NumRenderTargets);

    // Translate render targets
    for (uint32_t i = 0; i < NumRenderTargets; i++) {
        ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, pRenderTargets[i].cpuDescriptor);

        // Setup descriptor
        descriptors[i] = TextureDescriptor{
            .region = TextureRegion { },
            .uid = 0u
        };

        // Setup attachment
        AttachmentInfo& info = attachments[i];
        info.resource.token = token;
        info.resource.textureDescriptor = descriptors + i;

        // Translate action
        switch (pRenderTargets[i].BeginningAccess.Type) {
            default:
                ASSERT(false, "Invalid handle");
                break;
            case D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD:
                info.loadAction = AttachmentAction::Discard;
                break;
            case D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE:
                info.loadAction = AttachmentAction::Load;
                break;
            case D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR:
                info.loadAction = AttachmentAction::Clear;
                break;
            case D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS:
                info.loadAction = AttachmentAction::None;
                break;
        }

        // Translate action
        switch (pRenderTargets[i].EndingAccess.Type) {
            default:
                ASSERT(false, "Invalid handle");
                break;
            case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD:
                info.storeAction = AttachmentAction::Discard;
                break;
            case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE:
                info.storeAction = AttachmentAction::Store;
                break;
            case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE:
                info.storeAction = AttachmentAction::Resolve;
                break;
            case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS:
                info.storeAction = AttachmentAction::None;
                break;
        }
    }

    // Render pass info
    RenderPassInfo passInfo;
    passInfo.attachmentCount = NumRenderTargets;
    passInfo.attachments = attachments;

    // Optional depth data
    TextureDescriptor depthDescriptor;
    AttachmentInfo    depthInfo;

    if (pDepthStencil) {
        ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, pDepthStencil->cpuDescriptor);

        // Setup destination descriptor
        depthDescriptor = TextureDescriptor{
            .region = TextureRegion { },
            .uid = 0u
        };

        // Translate action
        switch (pDepthStencil->DepthBeginningAccess.Type) {
            default:
                ASSERT(false, "Invalid handle");
                break;
            case D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD:
                depthInfo.loadAction = AttachmentAction::Discard;
                break;
            case D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE:
                depthInfo.loadAction = AttachmentAction::Load;
                break;
            case D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR:
                depthInfo.loadAction = AttachmentAction::Clear;
                break;
            case D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS:
                depthInfo.loadAction = AttachmentAction::None;
                break;
        }

        // Translate action
        switch (pDepthStencil->DepthEndingAccess.Type) {
            default:
                ASSERT(false, "Invalid handle");
                break;
            case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD:
                depthInfo.storeAction = AttachmentAction::Discard;
                break;
            case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE:
                depthInfo.storeAction = AttachmentAction::Store;
                break;
            case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE:
                depthInfo.storeAction = AttachmentAction::Resolve;
                break;
            case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS:
                depthInfo.storeAction = AttachmentAction::None;
                break;
        }

        // Set resource info
        depthInfo.resource.token = token;
        depthInfo.resource.textureDescriptor = &depthDescriptor;
        passInfo.depthAttachment = &depthInfo;
    }

    // Invoke hook
    hook.Invoke(
        context,
        passInfo
    );
}

void FeatureHook_EndRenderPass::operator()(CommandListState *object, CommandContext *context) const {
    hook.Invoke(context);
}

void FeatureHook_OMSetRenderTargets::operator()(CommandListState *object, CommandContext *context, UINT NumRenderTargetDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors, BOOL RTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor) const {
    TextureDescriptor* descriptors = ALLOCA_ARRAY(TextureDescriptor, NumRenderTargetDescriptors);
    AttachmentInfo*    attachments = ALLOCA_ARRAY(AttachmentInfo, NumRenderTargetDescriptors);

    // Get device
    auto table = GetTable(object->parent);
    
    // Translate render targets
    for (uint32_t i = 0; i < NumRenderTargetDescriptors; i++) {
        D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle;
        if (RTsSingleHandleToDescriptorRange) {
            renderTargetHandle = {pRenderTargetDescriptors->ptr + table.next->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * i};
        } else {
            renderTargetHandle = pRenderTargetDescriptors[i];
        }

        // Get state
        ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, renderTargetHandle);

        // Setup descriptor
        descriptors[i] = TextureDescriptor{
            .region = TextureRegion { },
            .uid = 0u
        };

        // Setup attachment
        AttachmentInfo& info = attachments[i];
        info.resource.token = token;
        info.resource.textureDescriptor = descriptors + i;
        info.loadAction = AttachmentAction::Load;
        info.storeAction = AttachmentAction::Store;
    }

    // Render pass info
    RenderPassInfo passInfo;
    passInfo.attachmentCount = NumRenderTargetDescriptors;
    passInfo.attachments = attachments;

    // Optional depth data
    TextureDescriptor depthDescriptor;
    AttachmentInfo    depthInfo;

    if (pDepthStencilDescriptor) {
        ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, *pDepthStencilDescriptor);

        // Setup destination descriptor
        depthDescriptor = TextureDescriptor{
            .region = TextureRegion { },
            .uid = 0u
        };

        // Setup attachment
        depthInfo.resource.token = token;
        depthInfo.resource.textureDescriptor = &depthDescriptor;
        depthInfo.loadAction = AttachmentAction::Load;
        depthInfo.storeAction = AttachmentAction::Store;
        passInfo.depthAttachment = &depthInfo;
    }

    // Invoke hook
    hook.Invoke(
        context,
        passInfo
    );
}
