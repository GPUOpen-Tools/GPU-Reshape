#include <Backends/Vulkan/FeatureProxies.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/States/ImageState.h>

// Backend
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
