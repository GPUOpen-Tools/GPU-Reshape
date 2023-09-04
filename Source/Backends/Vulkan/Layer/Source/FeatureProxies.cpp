// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
#include <Backends/Vulkan/States/ImageState.h>

// Backend
#include <Backend/Command/AttachmentInfo.h>
#include <Backend/Command/RenderPassInfo.h>
#include <Backends/Vulkan/States/FrameBufferState.h>
#include <Backends/Vulkan/States/RenderPassState.h>

#include <Backend/Command/ResourceInfo.h>
#include <Backend/Command/BufferDescriptor.h>
#include <Backend/Command/TextureDescriptor.h>

/// Get a resource token
/// \param state object state
/// \return given token
static ResourceToken GetResourceToken(BufferState* state) {
    return ResourceToken {
        .puid = state->virtualMapping.puid,
        .type = static_cast<Backend::IL::ResourceTokenType>(state->virtualMapping.type),
        .srb= state->virtualMapping.srb
    };
}

/// Get a resource token
/// \param state object state
/// \return given token
static ResourceToken GetResourceToken(ImageState* state) {
    return ResourceToken {
        .puid = state->virtualMappingTemplate.puid,
        .type = static_cast<Backend::IL::ResourceTokenType>(state->virtualMappingTemplate.type),
        .srb= state->virtualMappingTemplate.srb
    };
}

/// Get a resource token
/// \param state object state
/// \return given token
static ResourceToken GetResourceToken(ImageViewState* state) {
    return ResourceToken {
        .puid = state->virtualMapping.puid,
        .type = static_cast<Backend::IL::ResourceTokenType>(state->virtualMapping.type),
        .srb= state->virtualMapping.srb
    };
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
            ResourceInfo::Buffer(GetResourceToken(srcBufferState), &srcDescriptor),
            ResourceInfo::Buffer(GetResourceToken(dstBufferState), &dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdCopyImage::operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions) const {
    // Get states
    ImageState* srcImageState = object->table->states_image.Get(srcImage);
    ImageState* dstImageState = object->table->states_image.Get(dstImage);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        // Setup source descriptor
        TextureDescriptor srcDescriptor{
            .region = {},
            .uid = srcImageState->uid
        };

        // Setup destination descriptor
        TextureDescriptor dstDescriptor{
            .region = {},
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(GetResourceToken(srcImageState), &srcDescriptor),
            ResourceInfo::Texture(GetResourceToken(dstImageState), &dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdBlitImage::operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter) const {
    // Get states
    ImageState* srcImageState = object->table->states_image.Get(srcImage);
    ImageState* dstImageState = object->table->states_image.Get(dstImage);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        // Setup source descriptor
        TextureDescriptor srcDescriptor{
            .region = {},
            .uid = srcImageState->uid
        };

        // Setup destination descriptor
        TextureDescriptor dstDescriptor{
            .region = {},
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(GetResourceToken(srcImageState), &srcDescriptor),
            ResourceInfo::Texture(GetResourceToken(dstImageState), &dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdCopyBufferToImage::operator()(CommandBufferObject *object, CommandContext *context, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions) const {
    // Get states
    BufferState* srcBufferState = object->table->states_buffer.Get(srcBuffer);
    ImageState*  dstImageState  = object->table->states_image.Get(dstImage);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        // Setup source descriptor
        BufferDescriptor srcDescriptor{
            .offset = pRegions[i].bufferOffset,
            .width = 0u,
            .uid = srcBufferState->uid
        };

        // Setup destination descriptor
        TextureDescriptor dstDescriptor{
            .region = {},
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Buffer(GetResourceToken(srcBufferState), &srcDescriptor),
            ResourceInfo::Texture(GetResourceToken(dstImageState), &dstDescriptor)
        );
    }
}

void FeatureHook_vkCmdCopyImageToBuffer::operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions) const {
    // Get states
    ImageState*  srcImageState  = object->table->states_image.Get(srcImage);
    BufferState* dstBufferState = object->table->states_buffer.Get(dstBuffer);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        // Setup source descriptor
        TextureDescriptor srcDescriptor{
            .region = {},
            .uid = srcImageState->uid
        };

        // Setup destination descriptor
        BufferDescriptor dstDescriptor{
            .offset = pRegions[i].bufferOffset,
            .width = 0u,
            .uid = dstBufferState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(GetResourceToken(srcImageState), &srcDescriptor),
            ResourceInfo::Buffer(GetResourceToken(dstBufferState), &dstDescriptor)
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
        ResourceInfo::Buffer(GetResourceToken(dstBufferState), &dstDescriptor)
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
        ResourceInfo::Buffer(GetResourceToken(dstBufferState), &dstDescriptor)
    );
}

void FeatureHook_vkCmdClearColorImage::operator()(CommandBufferObject *object, CommandContext *context, VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount, const VkImageSubresourceRange *pRanges) const {
    // Get states
    ImageState* dstImageState = object->table->states_image.Get(image);

    // Setup source descriptor
    TextureDescriptor dstDescriptor{
        .region = {},
        .uid = dstImageState->uid
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(GetResourceToken(dstImageState), &dstDescriptor)
    );
}

void FeatureHook_vkCmdClearDepthStencilImage::operator()(CommandBufferObject *object, CommandContext *context, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges) const {
    // Get states
    ImageState* dstImageState = object->table->states_image.Get(image);

    // Setup source descriptor
    TextureDescriptor dstDescriptor{
        .region = {},
        .uid = dstImageState->uid
    };

    // Invoke hook
    hook.Invoke(
        context,
        ResourceInfo::Texture(GetResourceToken(dstImageState), &dstDescriptor)
    );
}

void FeatureHook_vkCmdClearAttachments::operator()(CommandBufferObject *object, CommandContext *context, uint32_t attachmentCount, const VkClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects) const {
    // TODO: Render passes...
}

void FeatureHook_vkCmdResolveImage::operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions) const {
    // Get states
    ImageState* srcImageState = object->table->states_image.Get(srcImage);
    ImageState* dstImageState = object->table->states_image.Get(dstImage);

    // Invoke hook per region
    for (uint32_t i = 0; i < regionCount; i++) {
        // Setup source descriptor
        TextureDescriptor srcDescriptor{
            .region = {},
            .uid = srcImageState->uid
        };

        // Setup destination descriptor
        TextureDescriptor dstDescriptor{
            .region = {},
            .uid = dstImageState->uid
        };

        // Invoke hook
        hook.Invoke(
            context,
            ResourceInfo::Texture(GetResourceToken(srcImageState), &srcDescriptor),
            ResourceInfo::Texture(GetResourceToken(dstImageState), &dstDescriptor)
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
    
    // Translate render targets
    for (uint32_t i = 0; i < renderPassState->deepCopy.createInfo.attachmentCount; i++) {
        const VkAttachmentDescription2& description = renderPassState->deepCopy.createInfo.pAttachments[i];

        // Respective frame buffer view
        ImageViewState* state = frameBufferState->imageViews[i];

        // Setup descriptor
        descriptors[i] = TextureDescriptor{
            .region = TextureRegion { },
            .uid = 0u
        };

        // Setup attachment
        AttachmentInfo& attachmentInfo = attachments[i];
        attachmentInfo.resource.token = GetResourceToken(state);
        attachmentInfo.resource.textureDescriptor = descriptors + i;

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
