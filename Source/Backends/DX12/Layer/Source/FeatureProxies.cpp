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
#include <Backends/DX12/Translation.h>
#include <Backends/DX12/Table.Gen.h>

// Backend
#include <Backend/Resource/ResourceInfo.h>
#include <Backend/Command/RenderPassInfo.h>
#include <Backend/Command/AttachmentInfo.h>

/// Check if a resource is volumetric
static bool IsVolumetric(const ResourceState* state) {
    return state->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D;
}

/// Subresource indexing
struct SubresourceIndices {
    uint32_t mipIndex{0};
    uint32_t sliceIndex{0};
    uint32_t planeIndex{0};
};

/// Get all indices from a subresource index
/// \param index given index
/// \param mipCount total number of mips
/// \param sliceCount total number of slices
/// \param planeCount total number of planes
/// \return all indices
SubresourceIndices GetSubresourceIndices(uint32_t index, uint32_t mipCount, uint32_t sliceCount) {
    SubresourceIndices out;
    out.mipIndex = index % mipCount;
    out.sliceIndex = (index / mipCount) % sliceCount;
    out.planeIndex = index / (mipCount * sliceCount);
    return out;
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
static ResourceToken GetResourceTokenFromHeapHandle(CommandListState* state, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE handle, ResourceState** resourceState = nullptr) {
    auto table = GetTable(state->parent);

    // Get heap from handle ptr
    DescriptorHeapState* heap = table.state->cpuHeapTable.Find(type, handle.ptr);

    // Get virtual mapping
    VirtualResourceMapping virtualMapping;
    if (resourceState) {
        virtualMapping = heap->GetVirtualMappingFromHeapHandle(handle, resourceState);
    } else {
        virtualMapping = heap->GetVirtualMappingFromHeapHandle(handle);
    }

    // Get token
    return virtualMapping.token;
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
        ResourceInfo::Buffer(srcState->virtualMapping.token, srcDescriptor),
        ResourceInfo::Buffer(dstState->virtualMapping.token, dstDescriptor)
    );
}

static ResourceInfo GetSourceRegionResourceInfo(ResourceState* state, const D3D12_TEXTURE_COPY_LOCATION* pLocation, const D3D12_BOX* pSrcBox, DXGI_FORMAT format) {
    // Calculate source bounds
    uint32_t width = static_cast<uint32_t>(pSrcBox ? pSrcBox->right - pSrcBox->left : state->desc.Width);
    uint32_t height = pSrcBox ? pSrcBox->bottom - pSrcBox->top : state->desc.Height;
    uint32_t depth = pSrcBox ? pSrcBox->back - pSrcBox->front : state->desc.DepthOrArraySize;

    // Number of bytes for the format
    uint32_t formatByteCount = GetFormatByteSize(format);

    // Copying from buffer?
    if (state->desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        ASSERT(pLocation->Type == D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, "Expected placed footprint for buffers");

        // Source offset
        uint64_t offset = pLocation->PlacedFootprint.Offset;

        // Offset by the source box
        if (pSrcBox) {
            // TODO[init]: Block formats!
            uint32_t rowCount =  pLocation->PlacedFootprint.Footprint.Height;
            offset += pSrcBox->front * pLocation->PlacedFootprint.Footprint.RowPitch * rowCount;
            offset += pSrcBox->top * pLocation->PlacedFootprint.Footprint.RowPitch;
            offset += pSrcBox->left * formatByteCount;
        }

        // Create placed buffer
        return ResourceInfo::Buffer(state->virtualMapping.token, BufferDescriptor {
            .offset = offset,
            .width = state->desc.Width - offset,
            .placedDescriptor = BufferPlacedDescriptor {
                .rowLength = pLocation->PlacedFootprint.Footprint.RowPitch,
                .imageHeight = pLocation->PlacedFootprint.Footprint.Height,
            },
            .uid = 0u
        });
    } else {
        ASSERT(pLocation->Type == D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, "Expected sub-resource indices for textures");

        // Decompose indices
        SubresourceIndices indices = GetSubresourceIndices(pLocation->SubresourceIndex, state->desc.MipLevels, state->desc.DepthOrArraySize);

        // Setup texture region
        TextureRegion region {
            .baseMip = indices.mipIndex,
            .baseSlice = indices.sliceIndex,
            .offsetX = pSrcBox ? pSrcBox->left : 0,
            .offsetY = pSrcBox ? pSrcBox->top : 0,
            .offsetZ = pSrcBox ? pSrcBox->back : 0,
            .width = width,
            .height = height,
            .depth = depth
        };

        // Create descriptor
        return ResourceInfo::Texture(state->virtualMapping.token, IsVolumetric(state), TextureDescriptor {
            .region = region,
            .uid = 0u
        });
    }
}

static ResourceInfo GetDestRegionResourceInfo(ResourceState* state, const D3D12_TEXTURE_COPY_LOCATION* pLocation, uint32_t DstX, uint32_t DstY, uint32_t DstZ, const D3D12_BOX* pSrcBox, DXGI_FORMAT format) {
    // Calculate source bounds
    uint32_t width = static_cast<uint32_t>(pSrcBox ? pSrcBox->right - pSrcBox->left : state->desc.Width);
    uint32_t height = pSrcBox ? pSrcBox->bottom - pSrcBox->top : state->desc.Height;
    uint32_t depth = pSrcBox ? pSrcBox->back - pSrcBox->front : state->desc.DepthOrArraySize;

    // Number of bytes for the format
    uint32_t formatByteCount = GetFormatByteSize(format);

    // Copying from buffer?
    if (state->desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        ASSERT(pLocation->Type == D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, "Expected placed footprint for buffers");

        // Source offset
        uint64_t offset = pLocation->PlacedFootprint.Offset;

        // TODO[init]: Block formats!
        uint32_t rowCount =  pLocation->PlacedFootprint.Footprint.Height;
        offset += DstZ * pLocation->PlacedFootprint.Footprint.RowPitch * rowCount;
        offset += DstY * pLocation->PlacedFootprint.Footprint.RowPitch;
        offset += DstX * formatByteCount;

        // Create placed buffer
        return ResourceInfo::Buffer(state->virtualMapping.token, BufferDescriptor {
            .offset = offset,
            .width = state->desc.Width - offset,
            .placedDescriptor = BufferPlacedDescriptor {
                .rowLength = pLocation->PlacedFootprint.Footprint.RowPitch,
                .imageHeight = pLocation->PlacedFootprint.Footprint.Height,
            },
            .uid = 0u
        });
    } else {
        ASSERT(pLocation->Type == D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, "Expected sub-resource indices for textures");

        // Decompose indices
        SubresourceIndices indices = GetSubresourceIndices(pLocation->SubresourceIndex, state->desc.MipLevels, state->desc.DepthOrArraySize);

        // Setup region
        TextureRegion region {
            .baseMip = indices.mipIndex,
            .baseSlice = indices.sliceIndex,
            .offsetX = DstX,
            .offsetY = DstY,
            .offsetZ = DstZ,
            .width = width,
            .height = height,
            .depth = depth
        };

        // Create descriptor
        return ResourceInfo::Texture(state->virtualMapping.token, IsVolumetric(state), TextureDescriptor {
            .region = region,
            .uid = 0u
        });
    }
}

void FeatureHook_CopyTextureRegion::operator()(CommandListState *object, CommandContext *context, const D3D12_TEXTURE_COPY_LOCATION* pDst, UINT DstX, UINT DstY, UINT DstZ, const D3D12_TEXTURE_COPY_LOCATION* pSrc, const D3D12_BOX* pSrcBox) const {
    // Get states
    ResourceState* dstState = GetState(pDst->pResource);
    ResourceState* srcState = GetState(pSrc->pResource);

    // Determine the texture format
    DXGI_FORMAT format;
    if (srcState->desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        format = dstState->desc.Format;
    } else {
        format = srcState->desc.Format;
    }

    // Get infos
    ResourceInfo sourceInfo = GetSourceRegionResourceInfo(srcState, pSrc, pSrcBox, format);
    ResourceInfo destInfo = GetDestRegionResourceInfo(dstState, pDst, DstX, DstY, DstZ, pSrcBox, format);

    // Invoke hook
    hook.Invoke(context, sourceInfo, destInfo);
}

void FeatureHook_CopyResource::operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pDstResource, ID3D12Resource *pSrcResource) const {
    // Get states
    ResourceState* dstState = GetState(pDstResource);
    ResourceState* srcState = GetState(pSrcResource);

    // Handle type
    switch (static_cast<Backend::IL::ResourceTokenType>(dstState->virtualMapping.token.type)) {
        default: {
            ASSERT(false, "Unexpected type");
            break;
        }
        case Backend::IL::ResourceTokenType::Texture: {
            // Setup source descriptor
            TextureDescriptor srcDescriptor{
                .region = TextureRegion {
                    .mipCount = srcState->desc.MipLevels,
                    .width = static_cast<uint32_t>(srcState->desc.Width),
                    .height = srcState->desc.Height,
                    .depth = srcState->desc.DepthOrArraySize
                },
                .uid = 0u
            };

            // Setup destination descriptor
            TextureDescriptor dstDescriptor{
                .region = TextureRegion {
                    .mipCount = srcState->desc.MipLevels,
                    .width = static_cast<uint32_t>(dstState->desc.Width),
                    .height = dstState->desc.Height,
                    .depth = dstState->desc.DepthOrArraySize
                },
                .uid = 0u
            };

            // Invoke hook
            hook.Invoke(
                context,
                ResourceInfo::Texture(srcState->virtualMapping.token, IsVolumetric(srcState), srcDescriptor),
                ResourceInfo::Texture(dstState->virtualMapping.token, IsVolumetric(dstState), dstDescriptor)
            );
            break;
        }
        case Backend::IL::ResourceTokenType::Buffer: {
            // Setup source descriptor
            BufferDescriptor srcDescriptor{
                .offset = 0u,
                .width = srcState->desc.Width,
                .uid = 0u
            };

            // Setup destination descriptor
            BufferDescriptor dstDescriptor{
                .offset = 0u,
                .width = dstState->desc.Width,
                .uid = 0u
            };

            // Invoke hook
            hook.Invoke(
                context,
                ResourceInfo::Buffer(srcState->virtualMapping.token, srcDescriptor),
                ResourceInfo::Buffer(dstState->virtualMapping.token, dstDescriptor)
            );
            break;
        }
    }
}

void FeatureHook_ResolveSubresource::operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pDstResource, UINT DstSubresource, ID3D12Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format) const {
    // Get states
    ResourceState* dstState = GetState(pDstResource);
    ResourceState* srcState = GetState(pSrcResource);

    // Get indices from the subresources
    SubresourceIndices dstIndices = GetSubresourceIndices(DstSubresource, dstState->desc.MipLevels, dstState->desc.DepthOrArraySize);
    SubresourceIndices srcIndices = GetSubresourceIndices(SrcSubresource, srcState->desc.MipLevels, srcState->desc.DepthOrArraySize);

    // Setup source descriptor
    TextureDescriptor srcDescriptor{
        .region = TextureRegion {
            .baseMip = srcIndices.mipIndex,
            .baseSlice = srcIndices.sliceIndex,
            .width = static_cast<uint32_t>(srcState->desc.Width),
            .height = srcState->desc.Height,
            .depth = srcState->desc.DepthOrArraySize
        },
        .uid = 0u
    };

    // Setup destination descriptor
    TextureDescriptor dstDescriptor{
        .region = TextureRegion {
            .baseMip = dstIndices.mipIndex,
            .baseSlice = dstIndices.sliceIndex,
            .width = static_cast<uint32_t>(dstState->desc.Width),
            .height = dstState->desc.Height,
            .depth = dstState->desc.DepthOrArraySize
        },
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(srcState->virtualMapping.token, IsVolumetric(srcState), srcDescriptor),
        ResourceInfo::Texture(dstState->virtualMapping.token, IsVolumetric(dstState), dstDescriptor)
    );
}

void FeatureHook_ClearDepthStencilView::operator()(CommandListState *object, CommandContext *context, D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView, D3D12_CLEAR_FLAGS ClearFlags, FLOAT Depth, UINT8 Stencil, UINT NumRects, const D3D12_RECT *pRects) const {
    // Get states
    ResourceState* state{nullptr};
    ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DepthStencilView, &state);

    // Null rects imply full resource
    if (!pRects) {
        NumRects = 1;
    }
    
    for (uint32_t i = 0; i < NumRects; i++) {
        D3D12_RECT rect;

        // Assume rect from resource dimensions if default
        if (pRects) {
            rect = pRects[i];
        } else {
            rect = {
                .right = static_cast<LONG>(token.width),
                .bottom = static_cast<LONG>(token.height)
            };
        }
        
        // Setup source descriptor
        TextureDescriptor descriptor{
            .region = TextureRegion {
                .baseMip = token.viewBaseMip,
                .baseSlice = token.viewBaseSlice,
                .offsetX = static_cast<uint32_t>(rect.left),
                .offsetY = static_cast<uint32_t>(rect.top),
                .width = static_cast<uint32_t>(rect.right - rect.left),
                .height = static_cast<uint32_t>(rect.bottom - rect.top),
                .depth = token.viewSliceCount
            },
            .uid = 0u
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(token, IsVolumetric(state), descriptor)
        );
    }
}

void FeatureHook_ClearRenderTargetView::operator()(CommandListState *object, CommandContext *context, D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, const FLOAT *ColorRGBA, UINT NumRects, const D3D12_RECT *pRects) const {
    // Get states
    ResourceState* state{nullptr};
    ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RenderTargetView, &state);

    // Null rects imply full resource
    if (!pRects) {
        NumRects = 1;
    }
    
    for (uint32_t i = 0; i < NumRects; i++) {
        D3D12_RECT rect;

        // Assume rect from resource dimensions if default
        if (pRects) {
            rect = pRects[i];
        } else {
            rect = {
                .right = static_cast<LONG>(token.width),
                .bottom = static_cast<LONG>(token.height)
            };
        }
        
        // Setup source descriptor
        TextureDescriptor descriptor{
            .region = TextureRegion {
                .baseMip = token.viewBaseMip,
                .baseSlice = token.viewBaseSlice,
                .offsetX = static_cast<uint32_t>(rect.left),
                .offsetY = static_cast<uint32_t>(rect.top),
                .width = static_cast<uint32_t>(rect.right - rect.left),
                .height = static_cast<uint32_t>(rect.bottom - rect.top),
                .depth = token.viewSliceCount
            },
            .uid = 0u
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(token, IsVolumetric(state), descriptor)
        );
    }
}

void FeatureHook_ClearUnorderedAccessViewUint::operator()(CommandListState *object, CommandContext *context, D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap, D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle, ID3D12Resource *pResource, const UINT *Values, UINT NumRects, const D3D12_RECT *pRects) const {
    // Get states
    ResourceState* state{nullptr};
    ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, ViewCPUHandle, &state);

    // Null rects imply full resource
    if (!pRects) {
        NumRects = 1;
    }
    
    for (uint32_t i = 0; i < NumRects; i++) {
        D3D12_RECT rect;

        // Assume rect from resource dimensions if default
        if (pRects) {
            rect = pRects[i];
        } else {
            rect = {
                .right = static_cast<LONG>(token.width),
                .bottom = static_cast<LONG>(token.height)
            };
        }

        // Create resource info
        ResourceInfo info;
        if (token.GetType() == Backend::IL::ResourceTokenType::Buffer) {
            info = ResourceInfo::Buffer(token, BufferDescriptor{
                .offset = 0u,
                .width = token.width,
                .uid = 0u
            });
        } else {
            info = ResourceInfo::Texture(token, IsVolumetric(state), TextureDescriptor{
                .region = TextureRegion{
                    .baseMip = token.viewBaseMip,
                    .baseSlice = token.viewBaseSlice,
                    .offsetX = static_cast<uint32_t>(rect.left),
                    .offsetY = static_cast<uint32_t>(rect.top),
                    .width = static_cast<uint32_t>(rect.right - rect.left),
                    .height = static_cast<uint32_t>(rect.bottom - rect.top),
                    .depth = token.viewSliceCount
                },
                .uid = 0u
            });
        }

        // Invoke hook
        hook.Invoke(context, info);
    }
}

void FeatureHook_ClearUnorderedAccessViewFloat::operator()(CommandListState *object, CommandContext *context, D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap, D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle, ID3D12Resource *pResource, const FLOAT *Values, UINT NumRects, const D3D12_RECT *pRects) const {
    // Get states
    ResourceState* state{nullptr};
    ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, ViewCPUHandle, &state);

    // Null rects imply full resource
    if (!pRects) {
        NumRects = 1;
    }
    
    for (uint32_t i = 0; i < NumRects; i++) {
        D3D12_RECT rect;

        // Assume rect from resource dimensions if default
        if (pRects) {
            rect = pRects[i];
        } else {
            rect = {
                .right = static_cast<LONG>(token.width),
                .bottom = static_cast<LONG>(token.height)
            };
        }
        
        // Create resource info
        ResourceInfo info;
        if (token.GetType() == Backend::IL::ResourceTokenType::Buffer) {
            info = ResourceInfo::Buffer(token, BufferDescriptor{
                .offset = 0u,
                .width = token.width,
                .uid = 0u
            });
        } else {
            info = ResourceInfo::Texture(token, IsVolumetric(state), TextureDescriptor{
                .region = TextureRegion{
                    .baseMip = token.viewBaseMip,
                    .baseSlice = token.viewBaseSlice,
                    .offsetX = static_cast<uint32_t>(rect.left),
                    .offsetY = static_cast<uint32_t>(rect.top),
                    .width = static_cast<uint32_t>(rect.right - rect.left),
                    .height = static_cast<uint32_t>(rect.bottom - rect.top),
                    .depth = token.viewSliceCount
                },
                .uid = 0u
            });
        }

        // Invoke hook
        hook.Invoke(context, info);
    }
}

void FeatureHook_ResolveSubresourceRegion::operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, ID3D12Resource *pSrcResource, UINT SrcSubresource, D3D12_RECT *pSrcRect, DXGI_FORMAT Format, D3D12_RESOLVE_MODE ResolveMode) const {
    // Get states
    ResourceState* dstState = GetState(pDstResource);
    ResourceState* srcState = GetState(pSrcResource);

    // Get indices from the subresources
    SubresourceIndices dstIndices = GetSubresourceIndices(DstSubresource, dstState->desc.MipLevels, dstState->desc.DepthOrArraySize);
    SubresourceIndices srcIndices = GetSubresourceIndices(SrcSubresource, srcState->desc.MipLevels, srcState->desc.DepthOrArraySize);

    // Setup source descriptor
    TextureDescriptor srcDescriptor{
        .region = TextureRegion {
            .baseMip = srcIndices.mipIndex,
            .baseSlice = srcIndices.sliceIndex,
            .offsetX = static_cast<uint32_t>(pSrcRect ? pSrcRect->left : 0),
            .offsetY = static_cast<uint32_t>(pSrcRect ? pSrcRect->top : 0),
            .width = static_cast<uint32_t>(pSrcRect ? pSrcRect->right - pSrcRect->left : srcState->desc.Width),
            .height = pSrcRect ? pSrcRect->bottom - pSrcRect->top : srcState->desc.Height
        },
        .uid = 0u
    };

    // Setup destination descriptor
    TextureDescriptor dstDescriptor{
        .region = TextureRegion {
            .baseMip = dstIndices.mipIndex,
            .baseSlice = dstIndices.sliceIndex,
            .offsetX = DstX,
            .offsetY = DstY,
            .width = static_cast<uint32_t>(pSrcRect ? pSrcRect->right - pSrcRect->left : srcState->desc.Width),
            .height = pSrcRect ? pSrcRect->bottom - pSrcRect->top : srcState->desc.Height,
        },
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(srcState->virtualMapping.token, IsVolumetric(srcState), srcDescriptor),
        ResourceInfo::Texture(dstState->virtualMapping.token, IsVolumetric(dstState), dstDescriptor)
    );
}

void FeatureHook_DiscardResource::operator()(CommandListState *object, CommandContext *context, ID3D12Resource *pResource, const D3D12_DISCARD_REGION *pRegion) const {
    ResourceState* state = GetState(pResource);

    // Create resource info
    ResourceInfo info;
    if (state->virtualMapping.token.GetType() == Backend::IL::ResourceTokenType::Buffer) {
        info = ResourceInfo::Buffer(state->virtualMapping.token, BufferDescriptor{});
    } else {
        info = ResourceInfo::Texture(state->virtualMapping.token, IsVolumetric(state), TextureDescriptor{});
    }

    // Invoke hook
    hook.Invoke(context, info);
}

void FeatureHook_BeginRenderPass::operator()(CommandListState *object, CommandContext *context, UINT NumRenderTargets, const D3D12_RENDER_PASS_RENDER_TARGET_DESC *pRenderTargets, const D3D12_RENDER_PASS_DEPTH_STENCIL_DESC *pDepthStencil, D3D12_RENDER_PASS_FLAGS Flags) const {
    TextureDescriptor* descriptors = ALLOCA_ARRAY(TextureDescriptor, NumRenderTargets);
    AttachmentInfo*    attachments = ALLOCA_ARRAY(AttachmentInfo, NumRenderTargets);

    // Translate render targets
    for (uint32_t i = 0; i < NumRenderTargets; i++) {
        ResourceState* state{nullptr};
        ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, pRenderTargets[i].cpuDescriptor, &state);

        // Setup descriptor
        descriptors[i] = TextureDescriptor{
            .region = TextureRegion {
                .width  = token.width,
                .height = token.height,
                .depth  = token.depthOrSliceCount
            },
            .uid = 0u
        };

        // Setup attachment
        AttachmentInfo& info = attachments[i];
        info.resource.token = token;
        info.resource.textureDescriptor = descriptors[i];
        info.resource.isVolumetric = IsVolumetric(state);
        info.resolveResource = nullptr;

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
        ResourceState* state{nullptr};
        ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, pDepthStencil->cpuDescriptor, &state);

        // Setup destination descriptor
        depthDescriptor = TextureDescriptor{
            .region = TextureRegion {
                .width  = token.width,
                .height = token.height,
                .depth  = token.depthOrSliceCount
            },
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
        depthInfo.resource.textureDescriptor = depthDescriptor;
        depthInfo.resource.isVolumetric = IsVolumetric(state);
        depthInfo.resolveResource = nullptr;
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
        ResourceState* state{nullptr};
        ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, renderTargetHandle, &state);

        // Setup descriptor
        descriptors[i] = TextureDescriptor{
            .region = TextureRegion { 
                .width  = token.width,
                .height = token.height,
                .depth  = token.depthOrSliceCount
            },
            .uid = 0u
        };

        // Setup attachment
        AttachmentInfo& info = attachments[i];
        info.resource.token = token;
        info.resource.textureDescriptor = descriptors[i];
        info.resource.isVolumetric = IsVolumetric(state);
        info.loadAction = AttachmentAction::Load;
        info.storeAction = AttachmentAction::Store;
        info.resolveResource = nullptr;
    }

    // Render pass info
    RenderPassInfo passInfo;
    passInfo.attachmentCount = NumRenderTargetDescriptors;
    passInfo.attachments = attachments;

    // Optional depth data
    TextureDescriptor depthDescriptor;
    AttachmentInfo    depthInfo;

    if (pDepthStencilDescriptor) {
        ResourceState* state{nullptr};
        ResourceToken token = GetResourceTokenFromHeapHandle(object, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, *pDepthStencilDescriptor, &state);

        // Setup destination descriptor
        depthDescriptor = TextureDescriptor{
            .region = TextureRegion { 
                .width  = token.width,
                .height = token.height,
                .depth  = token.depthOrSliceCount
            },
            .uid = 0u
        };

        // Setup attachment
        depthInfo.resource.token = token;
        depthInfo.resource.textureDescriptor = depthDescriptor;
        depthInfo.resource.isVolumetric = IsVolumetric(state);
        depthInfo.loadAction = AttachmentAction::Load;
        depthInfo.storeAction = AttachmentAction::Store;
        depthInfo.resolveResource = nullptr;
        passInfo.depthAttachment = &depthInfo;
    }

    // Invoke hook
    hook.Invoke(
        context,
        passInfo
    );
}

void FeatureHook_CopyTiles::operator()(CommandListState *object, CommandContext *context, ID3D12Resource* pTiledResource, const D3D12_TILED_RESOURCE_COORDINATE* pTileRegionStartCoordinate, const D3D12_TILE_REGION_SIZE* pTileRegionSize, ID3D12Resource* pBuffer, UINT64 BufferStartOffsetInBytes, D3D12_TILE_COPY_FLAGS Flags) const {
#if 0 // todo[init]: Represent this somehow
    // Get resource infos
    ResourceInfo srcInfo = GetResourceInfoFor(GetState(pBuffer));
    ResourceInfo dstInfo = GetResourceInfoFor(GetState(pTiledResource));

    // Invoke proxies
    hook.Invoke(context, srcInfo, dstInfo);
#endif // 0
}
