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

struct FeatureHook_vkCmdCopyBuffer : TFeatureHook<Hooks::CopyBuffer> {
    void operator()(CommandBufferObject* object, CommandContext* context, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const;
};
