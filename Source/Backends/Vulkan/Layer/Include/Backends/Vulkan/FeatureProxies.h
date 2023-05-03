#pragma once

// Layer
#include <Backends/Vulkan/Layer.h>

// Backend
#include <Backend/FeatureHook.h>
#include <Backend/FeatureHookTable.h>

// Forward declarations
struct CommandBufferObject;

struct FeatureHook_vkCmdDraw : TFeatureHook<Hooks::DrawInstanced> {
    void operator()(CommandBufferObject* object, CommandContext* context, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const {
        hook.Invoke(context, vertexCount, instanceCount, firstVertex, firstInstance);
    }
};

struct FeatureHook_vkCmdDrawIndexed : TFeatureHook<Hooks::DrawIndexedInstanced> {
    void operator()(CommandBufferObject* object, CommandContext* context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const {
        hook.Invoke(context, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
};

struct FeatureHook_vkCmdDispatch : TFeatureHook<Hooks::Dispatch> {
    void operator()(CommandBufferObject* object, CommandContext* context, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const {
        hook.Invoke(context, groupCountX, groupCountY, groupCountZ);
    }
};

struct FeatureHook_vkCmdCopyBuffer : TFeatureHook<Hooks::CopyResource> {
    void operator()(CommandBufferObject* object, CommandContext* context, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const;
};

struct FeatureHook_vkCmdCopyImage : TFeatureHook<Hooks::CopyResource> {
    void operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions) const;
};

struct FeatureHook_vkCmdBlitImage : TFeatureHook<Hooks::CopyResource> {
    void operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) const;
};

struct FeatureHook_vkCmdCopyBufferToImage : TFeatureHook<Hooks::CopyResource> {
    void operator()(CommandBufferObject *object, CommandContext *context, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const;
};

struct FeatureHook_vkCmdCopyImageToBuffer : TFeatureHook<Hooks::CopyResource> {
    void operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions) const;
};

struct FeatureHook_vkCmdUpdateBuffer : TFeatureHook<Hooks::WriteResource> {
    void operator()(CommandBufferObject *object, CommandContext *context, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData) const;
};

struct FeatureHook_vkCmdFillBuffer : TFeatureHook<Hooks::WriteResource> {
    void operator()(CommandBufferObject *object, CommandContext *context, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) const;
};

struct FeatureHook_vkCmdClearColorImage : TFeatureHook<Hooks::ClearResource> {
    void operator()(CommandBufferObject *object, CommandContext *context, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges) const;
};

struct FeatureHook_vkCmdClearDepthStencilImage : TFeatureHook<Hooks::ClearResource> {
    void operator()(CommandBufferObject *object, CommandContext *context, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges) const;
};

struct FeatureHook_vkCmdClearAttachments : TFeatureHook<Hooks::ClearResource> {
    void operator()(CommandBufferObject *object, CommandContext *context, uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects) const;
};

struct FeatureHook_vkCmdResolveImage : TFeatureHook<Hooks::ResolveResource> {
    void operator()(CommandBufferObject *object, CommandContext *context, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions) const;
};

struct FeatureHook_vkCmdBeginRenderPass : TFeatureHook<Hooks::BeginRenderPass> {
    void operator()(CommandBufferObject *object, CommandContext *context, const VkRenderPassBeginInfo* info, VkSubpassContents contents) const;
};

struct FeatureHook_vkCmdEndRenderPass : TFeatureHook<Hooks::EndRenderPass> {
    void operator()(CommandBufferObject *object, CommandContext *context) const;
};
