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

#include <Backends/Vulkan/FeatureProxies.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/Export/StreamState.h>
#include <Backends/Vulkan/States/ImageState.h>

// Backend
#include <Backend/Command/AttachmentInfo.h>
#include <Backend/Command/RenderPassInfo.h>
#include <Backends/Vulkan/States/FrameBufferState.h>
#include <Backends/Vulkan/States/RenderPassState.h>
#include <Backends/Vulkan/Resource/ResourceInfo.h>
#include <Backend/Resource/ResourceInfo.h>

/// Check if a state is volumetric
static bool IsVolumetric(ImageState* state) {
    return state->createInfo.extent.depth > 1u;
}

/// Check if a state is volumetric
static bool IsVolumetric(ImageViewState* state) {
    return state->parent->createInfo.extent.depth > 1u;
}

void FeatureHook_vkCmdCopyBuffer::operator()(CommandBufferObject *object, CommandContext *context, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions) const {
    // Get states
    BufferState* srcBufferState = object->table->states_buffer.Get(srcBuffer);
    BufferState* dstBufferState = object->table->states_buffer.Get(dstBuffer);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        // Setup source descriptor
        BufferDescriptor srcDescriptor{
            .offset = pRegions[i].srcOffset,
            .width = pRegions[i].size,
            .uid = srcBufferState->uid
        };

        // Setup destination descriptor
        BufferDescriptor dstDescriptor{
            .offset = pRegions[i].dstOffset,
            .width = pRegions[i].size,
            .uid = dstBufferState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Buffer(srcBufferState->virtualMapping.token, srcDescriptor),
            ResourceInfo::Buffer(dstBufferState->virtualMapping.token, dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdCopyImage::operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions) const {
    // Get states
    ImageState* srcImageState = object->table->states_image.Get(srcImage);
    ImageState* dstImageState = object->table->states_image.Get(dstImage);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        const VkImageCopy& region = pRegions[i];

        // Expand the subresource for mappings
        VkImageSubresourceLayers srcSubresource = ExpandImageSubresourceLayers(srcImageState, region.srcSubresource);
        VkImageSubresourceLayers dstSubresource = ExpandImageSubresourceLayers(dstImageState, region.dstSubresource);
        
        // Setup source descriptor
        TextureDescriptor srcDescriptor{
            .region = {
                .baseMip = srcSubresource.mipLevel,
                .baseSlice = srcSubresource.baseArrayLayer,
                .offsetX = static_cast<uint32_t>(region.srcOffset.x),
                .offsetY = static_cast<uint32_t>(region.srcOffset.y),
                .offsetZ = static_cast<uint32_t>(region.srcOffset.z),
                .width = region.extent.width,
                .height = region.extent.height,
                .depth = std::max(region.extent.depth, srcSubresource.layerCount)
            },
            .uid = srcImageState->uid
        };

        // Setup destination descriptor
        TextureDescriptor dstDescriptor{
            .region = {
                .baseMip = dstSubresource.mipLevel,
                .baseSlice = dstSubresource.baseArrayLayer,
                .offsetX = static_cast<uint32_t>(region.dstOffset.x),
                .offsetY = static_cast<uint32_t>(region.dstOffset.y),
                .offsetZ = static_cast<uint32_t>(region.dstOffset.z),
                .width = region.extent.width,
                .height = region.extent.height,
                .depth = std::max(region.extent.depth, dstSubresource.baseArrayLayer + dstSubresource.layerCount)
            },
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(srcImageState->virtualMappingTemplate.token, IsVolumetric(srcImageState), srcDescriptor),
            ResourceInfo::Texture(dstImageState->virtualMappingTemplate.token, IsVolumetric(dstImageState), dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdBlitImage::operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter) const {
    // Get states
    ImageState* srcImageState = object->table->states_image.Get(srcImage);
    ImageState* dstImageState = object->table->states_image.Get(dstImage);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        const VkImageBlit& region = pRegions[i];

        // Expand the subresource for mappings
        VkImageSubresourceLayers srcSubresource = ExpandImageSubresourceLayers(srcImageState, region.srcSubresource);
        VkImageSubresourceLayers dstSubresource = ExpandImageSubresourceLayers(dstImageState, region.dstSubresource);
        
        // Setup source descriptor
        TextureDescriptor srcDescriptor{
            .region = {
                .baseMip = srcSubresource.mipLevel,
                .baseSlice = srcSubresource.baseArrayLayer,
                .offsetX = static_cast<uint32_t>(region.srcOffsets[0].x),
                .offsetY = static_cast<uint32_t>(region.srcOffsets[0].y),
                .offsetZ = static_cast<uint32_t>(region.srcOffsets[0].z),
                .width = static_cast<uint32_t>(region.srcOffsets[1].x - region.srcOffsets[0].x),
                .height = static_cast<uint32_t>(region.srcOffsets[1].y - region.srcOffsets[0].y),
                .depth = static_cast<uint32_t>(region.srcOffsets[1].z - region.srcOffsets[0].z)
            },
            .uid = srcImageState->uid
        };

        // Setup destination descriptor
        TextureDescriptor dstDescriptor{
            .region = {
                .baseMip = dstSubresource.mipLevel,
                .baseSlice = dstSubresource.baseArrayLayer,
                .offsetX = static_cast<uint32_t>(region.dstOffsets[0].x),
                .offsetY = static_cast<uint32_t>(region.dstOffsets[0].y),
                .offsetZ = static_cast<uint32_t>(region.dstOffsets[0].z),
                .width = static_cast<uint32_t>(region.dstOffsets[1].x - region.dstOffsets[0].x),
                .height = static_cast<uint32_t>(region.dstOffsets[1].y - region.dstOffsets[0].y),
                .depth = static_cast<uint32_t>(region.dstOffsets[1].z - region.dstOffsets[0].z)
            },
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(srcImageState->virtualMappingTemplate.token, IsVolumetric(srcImageState), srcDescriptor),
            ResourceInfo::Texture(dstImageState->virtualMappingTemplate.token, IsVolumetric(dstImageState), dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdCopyBufferToImage::operator()(CommandBufferObject *object, CommandContext *context, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions) const {
    // Get states
    BufferState* srcBufferState = object->table->states_buffer.Get(srcBuffer);
    ImageState*  dstImageState  = object->table->states_image.Get(dstImage);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        const VkBufferImageCopy& region = pRegions[i];

        // Expand the subresource for mappings
        VkImageSubresourceLayers dstSubresource = ExpandImageSubresourceLayers(dstImageState, region.imageSubresource);

        // Setup source descriptor
        BufferDescriptor srcDescriptor{
            .offset = region.bufferOffset,
            .width = srcBufferState->virtualMapping.token.width - region.bufferOffset,
            .placedDescriptor = GetBufferPlacedDescriptor(dstImageState, region.bufferRowLength, region.bufferImageHeight),
            .uid = srcBufferState->uid
        };

        // Setup destination descriptor
        TextureDescriptor dstDescriptor{
            .region = {
                .baseMip = dstSubresource.mipLevel,
                .baseSlice = dstSubresource.baseArrayLayer,
                .offsetX = static_cast<uint32_t>(region.imageOffset.x),
                .offsetY = static_cast<uint32_t>(region.imageOffset.y),
                .offsetZ = static_cast<uint32_t>(region.imageOffset.z),
                .width = region.imageExtent.width,
                .height = region.imageExtent.height,
                .depth = std::max(dstSubresource.baseArrayLayer + dstSubresource.layerCount, region.imageExtent.depth)
            },
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Buffer(srcBufferState->virtualMapping.token, srcDescriptor),
            ResourceInfo::Texture(dstImageState->virtualMappingTemplate.token, IsVolumetric(dstImageState), dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdCopyBufferToImage2::operator()(CommandBufferObject *object, CommandContext *context, const VkCopyBufferToImageInfo2* pCopyBufferToImageInfo) const {
    // Get states
    BufferState* srcBufferState = object->table->states_buffer.Get(pCopyBufferToImageInfo->srcBuffer);
    ImageState*  dstImageState  = object->table->states_image.Get(pCopyBufferToImageInfo->dstImage);

    // Invoke hook per region
    for (uint32_t i = 0; i < pCopyBufferToImageInfo->regionCount; i++) {
        const VkBufferImageCopy2& region = pCopyBufferToImageInfo->pRegions[i];

        // Expand the subresource for mappings
        VkImageSubresourceLayers dstSubresource = ExpandImageSubresourceLayers(dstImageState, region.imageSubresource);

        // Setup source descriptor
        BufferDescriptor srcDescriptor{
            .offset = region.bufferOffset,
            .width = srcBufferState->virtualMapping.token.width - region.bufferOffset,
            .placedDescriptor = GetBufferPlacedDescriptor(dstImageState, region.bufferRowLength, region.bufferImageHeight),
            .uid = srcBufferState->uid
        };

        // Setup destination descriptor
        TextureDescriptor dstDescriptor{
            .region = {
                .baseMip = dstSubresource.mipLevel,
                .baseSlice = dstSubresource.baseArrayLayer,
                .offsetX = static_cast<uint32_t>(region.imageOffset.x),
                .offsetY = static_cast<uint32_t>(region.imageOffset.y),
                .offsetZ = static_cast<uint32_t>(region.imageOffset.z),
                .width = region.imageExtent.width,
                .height = region.imageExtent.height,
                .depth = std::max(dstSubresource.baseArrayLayer + dstSubresource.layerCount, region.imageExtent.depth)
            },
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Buffer(srcBufferState->virtualMapping.token, srcDescriptor),
            ResourceInfo::Texture(dstImageState->virtualMappingTemplate.token, IsVolumetric(dstImageState), dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdCopyImageToBuffer::operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions) const {
    // Get states
    ImageState*  srcImageState  = object->table->states_image.Get(srcImage);
    BufferState* dstBufferState = object->table->states_buffer.Get(dstBuffer);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        const VkBufferImageCopy& region = pRegions[i];

        // Expand the subresource for mappings
        VkImageSubresourceLayers srcSubresource = ExpandImageSubresourceLayers(srcImageState, region.imageSubresource);

        // Setup source descriptor
        TextureDescriptor srcDescriptor{
            .region = {
                .baseMip = srcSubresource.mipLevel,
                .baseSlice = srcSubresource.baseArrayLayer,
                .offsetX = static_cast<uint32_t>(region.imageOffset.x),
                .offsetY = static_cast<uint32_t>(region.imageOffset.y),
                .offsetZ = static_cast<uint32_t>(region.imageOffset.z),
                .width = region.imageExtent.width,
                .height = region.imageExtent.height,
                .depth = std::max(srcSubresource.baseArrayLayer + srcSubresource.layerCount, region.imageExtent.depth)
            },
            .uid = srcImageState->uid
        };

        // Setup destination descriptor
        BufferDescriptor dstDescriptor{
            .offset = region.bufferOffset,
            .width = dstBufferState->virtualMapping.token.width - region.bufferOffset,
            .placedDescriptor = GetBufferPlacedDescriptor(srcImageState, region.bufferRowLength, region.bufferImageHeight),
            .uid = dstBufferState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(srcImageState->virtualMappingTemplate.token, IsVolumetric(srcImageState), srcDescriptor),
            ResourceInfo::Buffer(dstBufferState->virtualMapping.token, dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdCopyImageToBuffer2::operator()(CommandBufferObject *object, CommandContext *context, const VkCopyImageToBufferInfo2* pCopyImageToBufferInfo) const {
    // Get states
    ImageState*  srcImageState  = object->table->states_image.Get(pCopyImageToBufferInfo->srcImage);
    BufferState* dstBufferState = object->table->states_buffer.Get(pCopyImageToBufferInfo->dstBuffer);

    // Invoke hook per region
    for (uint32_t i = 0; i < pCopyImageToBufferInfo->regionCount; i++) {
        const VkBufferImageCopy2& region = pCopyImageToBufferInfo->pRegions[i];

        // Expand the subresource for mappings
        VkImageSubresourceLayers srcSubresource = ExpandImageSubresourceLayers(srcImageState, region.imageSubresource);

        // Setup source descriptor
        TextureDescriptor srcDescriptor{
            .region = {
                .baseMip = srcSubresource.mipLevel,
                .baseSlice = srcSubresource.baseArrayLayer,
                .offsetX = static_cast<uint32_t>(region.imageOffset.x),
                .offsetY = static_cast<uint32_t>(region.imageOffset.y),
                .offsetZ = static_cast<uint32_t>(region.imageOffset.z),
                .width = region.imageExtent.width,
                .height = region.imageExtent.height,
                .depth = std::max(srcSubresource.baseArrayLayer + srcSubresource.layerCount, region.imageExtent.depth)
            },
            .uid = srcImageState->uid
        };

        // Setup destination descriptor
        BufferDescriptor dstDescriptor{
            .offset = region.bufferOffset,
            .width = dstBufferState->virtualMapping.token.width - region.bufferOffset,
            .placedDescriptor = GetBufferPlacedDescriptor(srcImageState, region.bufferRowLength, region.bufferImageHeight),
            .uid = dstBufferState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(srcImageState->virtualMappingTemplate.token, IsVolumetric(srcImageState), srcDescriptor),
            ResourceInfo::Buffer(dstBufferState->virtualMapping.token, dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdUpdateBuffer::operator()(CommandBufferObject *object, CommandContext *context, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData) const {
    // Get states
    BufferState* dstBufferState = object->table->states_buffer.Get(dstBuffer);

    // Setup source descriptor
    BufferDescriptor dstDescriptor{
        .offset = dstOffset,
        .width = dataSize,
        .uid = dstBufferState->uid
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Buffer(dstBufferState->virtualMapping.token, dstDescriptor)
    );
}

void FeatureHook_vkCmdFillBuffer::operator()(CommandBufferObject *object, CommandContext *context, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) const {
    // Get states
    BufferState* dstBufferState = object->table->states_buffer.Get(dstBuffer);

    // Setup source descriptor
    BufferDescriptor dstDescriptor{
        .offset = dstOffset,
        .width = size,
        .uid = dstBufferState->uid
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Buffer(dstBufferState->virtualMapping.token, dstDescriptor)
    );
}

void FeatureHook_vkCmdClearColorImage::operator()(CommandBufferObject *object, CommandContext *context, VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount, const VkImageSubresourceRange *pRanges) const {
    // Get states
    ImageState* dstImageState = object->table->states_image.Get(image);

    // Invoke hook per region
    for (uint32_t i = 0; i < rangeCount; i++) {
        VkImageSubresourceRange region = ExpandImageSubresourceRange(dstImageState, pRanges[i]);
        
        // Setup source descriptor
        TextureDescriptor dstDescriptor{
            .region = {
                .baseMip = region.baseMipLevel,
                .baseSlice = region.baseArrayLayer,
                .mipCount = region.levelCount,
                .width = dstImageState->createInfo.extent.width,
                .height = dstImageState->createInfo.extent.height,
                .depth = std::max(region.baseArrayLayer + region.layerCount, dstImageState->createInfo.extent.depth)
            },
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(dstImageState->virtualMappingTemplate.token, IsVolumetric(dstImageState), dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdClearDepthStencilImage::operator()(CommandBufferObject *object, CommandContext *context, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges) const {
    // Get states
    ImageState* dstImageState = object->table->states_image.Get(image);

    // Invoke hook per region
    for (uint32_t i = 0; i < rangeCount; i++) {
        VkImageSubresourceRange region = ExpandImageSubresourceRange(dstImageState, pRanges[i]);
        
        // Setup source descriptor
        TextureDescriptor dstDescriptor{
            .region = {
                .baseMip = region.baseMipLevel,
                .baseSlice = region.baseArrayLayer,
                .mipCount = region.levelCount,
                .width = dstImageState->createInfo.extent.width,
                .height = dstImageState->createInfo.extent.height,
                .depth = std::max(region.baseArrayLayer + region.layerCount, dstImageState->createInfo.extent.depth)
            },
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(dstImageState->virtualMappingTemplate.token, IsVolumetric(dstImageState), dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdClearAttachments::operator()(CommandBufferObject *object, CommandContext *context, uint32_t attachmentCount, const VkClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects) const {
    ASSERT(object->streamState->renderPass.insideRenderPass, "Unexpected state");

    // Get states
    FrameBufferState* frameBufferState = object->table->states_frameBuffers.Get(object->streamState->renderPass.deepCopy->framebuffer);

    // Clear each attachment
    for (uint32_t attachmentIndex = 0; attachmentIndex < attachmentCount; attachmentIndex++) {
        ImageViewState* imageViewState = frameBufferState->imageViews[attachmentIndex];

        // Clear each rect area
        for (uint32_t regionIndex = 0; regionIndex < rectCount; regionIndex++) {
            const VkClearRect& rect = pRects[regionIndex];
        
            // Setup source descriptor
            TextureDescriptor dstDescriptor{
                .region = {
                    .baseSlice = rect.baseArrayLayer,
                    .width = imageViewState->parent->createInfo.extent.width,
                    .height = imageViewState->parent->createInfo.extent.height,
                    .depth = std::max(rect.baseArrayLayer + rect.layerCount, imageViewState->parent->createInfo.extent.depth)
                },
                .uid = imageViewState->uid
            };

            // Invoke hook
            hook.Invoke(
                context,
                ResourceInfo::Texture(imageViewState->virtualMapping.token, IsVolumetric(imageViewState), dstDescriptor)
            );
        }
    }
}

void FeatureHook_vkCmdResolveImage::operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions) const {
    // Get states
    ImageState* srcImageState = object->table->states_image.Get(srcImage);
    ImageState* dstImageState = object->table->states_image.Get(dstImage);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        const VkImageResolve& region = pRegions[i];

        // Expand the subresource for mappings
        VkImageSubresourceLayers srcSubresource = ExpandImageSubresourceLayers(srcImageState, region.srcSubresource);
        VkImageSubresourceLayers dstSubresource = ExpandImageSubresourceLayers(dstImageState, region.dstSubresource);
        
        // Setup source descriptor
        TextureDescriptor srcDescriptor{
            .region = {
                .baseMip = srcSubresource.mipLevel,
                .baseSlice = srcSubresource.baseArrayLayer,
                .offsetX = static_cast<uint32_t>(region.srcOffset.x),
                .offsetY = static_cast<uint32_t>(region.srcOffset.y),
                .offsetZ = static_cast<uint32_t>(region.srcOffset.z),
                .width = region.extent.width,
                .height = region.extent.height,
                .depth = std::max(srcSubresource.baseArrayLayer + srcSubresource.layerCount, region.extent.depth)
            },
            .uid = srcImageState->uid
        };

        // Setup destination descriptor
        TextureDescriptor dstDescriptor{
            .region = {
                .baseMip = dstSubresource.mipLevel,
                .baseSlice = dstSubresource.baseArrayLayer,
                .offsetX = static_cast<uint32_t>(region.dstOffset.x),
                .offsetY = static_cast<uint32_t>(region.dstOffset.y),
                .offsetZ = static_cast<uint32_t>(region.dstOffset.z),
                .width = region.extent.width,
                .height = region.extent.height,
                .depth = std::max(dstSubresource.baseArrayLayer + dstSubresource.layerCount, region.extent.depth)
            },
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(srcImageState->virtualMappingTemplate.token, IsVolumetric(srcImageState), srcDescriptor),
            ResourceInfo::Texture(dstImageState->virtualMappingTemplate.token, IsVolumetric(dstImageState), dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdBeginRenderPass::operator()(CommandBufferObject *object, CommandContext *context, const VkRenderPassBeginInfo *info, VkSubpassContents contents) const {    
    // Get states
    FrameBufferState* frameBufferState = object->table->states_frameBuffers.Get(info->framebuffer);
    RenderPassState* renderPassState = object->table->states_renderPass.Get(info->renderPass);

    // Local data
    TextureDescriptor* descriptors = ALLOCA_ARRAY(TextureDescriptor, renderPassState->deepCopy->attachmentCount);
    AttachmentInfo*    attachments = ALLOCA_ARRAY(AttachmentInfo, renderPassState->deepCopy->attachmentCount);

    // Optional infos
    auto* attachmentBeginInfo = FindStructureTypeSafe<VkRenderPassAttachmentBeginInfo>(info, VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO);
    
    // Translate render targets
    for (uint32_t i = 0; i < renderPassState->deepCopy.createInfo.attachmentCount; i++) {
        const VkAttachmentDescription2& description = renderPassState->deepCopy.createInfo.pAttachments[i];

        // Respective frame buffer view
        ImageViewState* state;
        if (attachmentBeginInfo) {
            state = object->table->states_imageView.Get(attachmentBeginInfo->pAttachments[i]);
        } else {
            state = frameBufferState->imageViews[i];
        }

        // Setup descriptor
        descriptors[i] = TextureDescriptor{
            .region = TextureRegion {
                .width = state->virtualMapping.token.width,
                .height = state->virtualMapping.token.height,
                .depth = state->virtualMapping.token.depthOrSliceCount
             },
            .uid = 0u
        };

        // Setup attachment
        AttachmentInfo& attachmentInfo = attachments[i];
        attachmentInfo.resource.token = state->virtualMapping.token;
        attachmentInfo.resource.isVolumetric = IsVolumetric(state);
        attachmentInfo.resource.textureDescriptor = descriptors[i];
        attachmentInfo.resolveResource = nullptr;

        // Translate action
        switch (description.loadOp) {
            default:
                ASSERT(false, "Invalid handle");
                break;
            case VK_ATTACHMENT_LOAD_OP_LOAD:
                attachmentInfo.loadAction = AttachmentAction::Load;
                break;
            case VK_ATTACHMENT_LOAD_OP_CLEAR:
                attachmentInfo.loadAction = AttachmentAction::Clear;
                break;
            case VK_ATTACHMENT_LOAD_OP_DONT_CARE:
                attachmentInfo.loadAction = AttachmentAction::Discard;
                break;
            case VK_ATTACHMENT_LOAD_OP_NONE_EXT:
                attachmentInfo.loadAction = AttachmentAction::None;
                break;
        }

        // Translate action
        switch (description.storeOp) {
            default:
                ASSERT(false, "Invalid handle");
                break;
            case VK_ATTACHMENT_STORE_OP_STORE:
                attachmentInfo.storeAction = AttachmentAction::Store;
                break;
            case VK_ATTACHMENT_STORE_OP_DONT_CARE:
                attachmentInfo.storeAction = AttachmentAction::Discard;
                break;
            case VK_ATTACHMENT_STORE_OP_NONE:
                attachmentInfo.storeAction = AttachmentAction::None;
                break;
        }
    }

    // Render pass info
    RenderPassInfo passInfo;
    passInfo.attachmentCount = renderPassState->deepCopy.createInfo.attachmentCount;
    passInfo.attachments = attachments;

    // Invoke hook
    hook.Invoke(
        context,
        passInfo
    );
}

void FeatureHook_vkCmdEndRenderPass::operator()(CommandBufferObject *object, CommandContext *context) const {
    hook.Invoke(context);
}

void FeatureHook_vkCmdBeginRenderPass2::operator()(CommandBufferObject *object, CommandContext *context, const VkRenderPassBeginInfo *info, const VkSubpassBeginInfo *pSubpassBeginInfo) const {
    FeatureHook_vkCmdBeginRenderPass{hook}(object, context, info, { });
}

void FeatureHook_vkCmdBeginRendering::operator()(CommandBufferObject *object, CommandContext *context, const VkRenderingInfo *pRenderingInfo) const {
    // Local data
    TextureDescriptor* descriptors  = ALLOCA_ARRAY(TextureDescriptor, pRenderingInfo->colorAttachmentCount);
    TextureDescriptor* resolveDescriptors = ALLOCA_ARRAY(TextureDescriptor, pRenderingInfo->colorAttachmentCount);
    AttachmentInfo*    attachments  = ALLOCA_ARRAY(AttachmentInfo, pRenderingInfo->colorAttachmentCount);
    ResourceInfo*      resolveInfos = ALLOCA_ARRAY(ResourceInfo, pRenderingInfo->colorAttachmentCount);
    
    // Translate render targets
    for (uint32_t i = 0; i < pRenderingInfo->colorAttachmentCount; i++) {
        const VkRenderingAttachmentInfo& description = pRenderingInfo->pColorAttachments[i];

        // Respective frame buffer view
        ImageViewState* state = object->table->states_imageView.Get(description.imageView);

        // Setup descriptor
        descriptors[i] = TextureDescriptor{
            .region = TextureRegion {
                .width = state->virtualMapping.token.width,
                .height = state->virtualMapping.token.height,
                .depth = state->virtualMapping.token.depthOrSliceCount
            },
            .uid = 0u
        };

        // Setup attachment
        AttachmentInfo& attachmentInfo = attachments[i];
        attachmentInfo.resource.token = state->virtualMapping.token;
        attachmentInfo.resource.isVolumetric = IsVolumetric(state);
        attachmentInfo.resource.textureDescriptor = descriptors[i];

        // Has resolve target?
        if (description.resolveImageView) {
            // Respective frame buffer view
            ImageViewState* resolveState = object->table->states_imageView.Get(description.resolveImageView);

            // Setup descriptor
            resolveDescriptors[i] = TextureDescriptor{
                .region = TextureRegion {
                    .width = state->virtualMapping.token.width,
                    .height = state->virtualMapping.token.height,
                    .depth = state->virtualMapping.token.depthOrSliceCount
                },
                .uid = 0u
            };

            // Setup attachment resource
            ResourceInfo& resolve = resolveInfos[i];
            resolve.token = resolveState->virtualMapping.token;
            resolve.isVolumetric = IsVolumetric(resolveState);
            resolve.textureDescriptor = resolveDescriptors[i];
            attachmentInfo.resolveResource = resolveInfos + i;
        } else {
            attachmentInfo.resolveResource = nullptr;
        }

        // Translate action
        switch (description.loadOp) {
            default:
                ASSERT(false, "Invalid handle");
                break;
            case VK_ATTACHMENT_LOAD_OP_LOAD:
                attachmentInfo.loadAction = AttachmentAction::Load;
                break;
            case VK_ATTACHMENT_LOAD_OP_CLEAR:
                attachmentInfo.loadAction = AttachmentAction::Clear;
                break;
            case VK_ATTACHMENT_LOAD_OP_DONT_CARE:
                attachmentInfo.loadAction = AttachmentAction::Discard;
                break;
            case VK_ATTACHMENT_LOAD_OP_NONE_EXT:
                attachmentInfo.loadAction = AttachmentAction::None;
                break;
        }

        // Translate action
        switch (description.storeOp) {
            default:
                ASSERT(false, "Invalid handle");
                break;
            case VK_ATTACHMENT_STORE_OP_STORE:
                attachmentInfo.storeAction = AttachmentAction::Store;
                break;
            case VK_ATTACHMENT_STORE_OP_DONT_CARE:
                attachmentInfo.storeAction = AttachmentAction::Discard;
                break;
            case VK_ATTACHMENT_STORE_OP_NONE:
                attachmentInfo.storeAction = AttachmentAction::None;
                break;
        }
    }

    // Render pass info
    RenderPassInfo passInfo;
    passInfo.attachmentCount = pRenderingInfo->colorAttachmentCount;
    passInfo.attachments = attachments;

    // Optional depth data
    TextureDescriptor depthDescriptor;
    TextureDescriptor depthResolveDescriptor;
    AttachmentInfo    depthInfo;
    ResourceInfo      depthResolveInfo;
    
    if (pRenderingInfo->pDepthAttachment) {
        // Respective frame buffer view
        ImageViewState* depthState = object->table->states_imageView.Get(pRenderingInfo->pDepthAttachment->imageView);

        // Setup descriptor
        depthDescriptor = TextureDescriptor{
            .region = TextureRegion { 
                .width = depthState->virtualMapping.token.width,
                .height = depthState->virtualMapping.token.height,
                .depth = depthState->virtualMapping.token.depthOrSliceCount
            },
            .uid = 0u
        };

        // Has resolve target?
        if (pRenderingInfo->pDepthAttachment->resolveImageView) {
            // Respective frame buffer view
            ImageViewState* resolveState = object->table->states_imageView.Get(pRenderingInfo->pDepthAttachment->resolveImageView);

            // Setup descriptor
            depthResolveDescriptor = TextureDescriptor{
                .region = TextureRegion { 
                    .width = resolveState->virtualMapping.token.width,
                    .height = resolveState->virtualMapping.token.height,
                    .depth = resolveState->virtualMapping.token.depthOrSliceCount
                },
                .uid = 0u
            };

            // Setup attachment resource
            depthResolveInfo.token = resolveState->virtualMapping.token;
            depthResolveInfo.isVolumetric = IsVolumetric(resolveState);
            depthResolveInfo.textureDescriptor = depthResolveDescriptor;
            depthInfo.resolveResource = &depthResolveInfo;
        } else {
            depthInfo.resolveResource = nullptr;
        }

        // Translate action
        switch (pRenderingInfo->pDepthAttachment->loadOp) {
            default:
                ASSERT(false, "Invalid handle");
                break;
            case VK_ATTACHMENT_LOAD_OP_LOAD:
                depthInfo.loadAction = AttachmentAction::Load;
                break;
            case VK_ATTACHMENT_LOAD_OP_CLEAR:
                depthInfo.loadAction = AttachmentAction::Clear;
                break;
            case VK_ATTACHMENT_LOAD_OP_DONT_CARE:
                depthInfo.loadAction = AttachmentAction::Discard;
                break;
            case VK_ATTACHMENT_LOAD_OP_NONE_EXT:
                depthInfo.loadAction = AttachmentAction::None;
                break;
        }

        // Translate action
        switch (pRenderingInfo->pDepthAttachment->storeOp) {
            default:
                ASSERT(false, "Invalid handle");
                break;
            case VK_ATTACHMENT_STORE_OP_STORE:
                depthInfo.storeAction = AttachmentAction::Store;
                break;
            case VK_ATTACHMENT_STORE_OP_DONT_CARE:
                depthInfo.storeAction = AttachmentAction::Discard;
                break;
            case VK_ATTACHMENT_STORE_OP_NONE:
                depthInfo.storeAction = AttachmentAction::None;
                break;
        }

        // Set resource info
        depthInfo.resource.token = depthState->virtualMapping.token;
        depthInfo.resource.isVolumetric = IsVolumetric(depthState);
        depthInfo.resource.textureDescriptor = depthDescriptor;
        passInfo.depthAttachment = &depthInfo;
    }
    
    // Invoke hook
    hook.Invoke(
        context,
        passInfo
    );
}

void FeatureHook_vkCmdEndRendering::operator()(CommandBufferObject *object, CommandContext *context) const {
    hook.Invoke(context);
}
