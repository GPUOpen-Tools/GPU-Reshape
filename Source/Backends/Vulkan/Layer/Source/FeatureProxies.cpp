#include <Backends/Vulkan/FeatureProxies.h>
#include <Backends/Vulkan/Objects/CommandBufferObject.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backend/Command/ResourceInfo.h>

#include "Backend/Command/BufferDescriptor.h"

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

        // Source info
        ResourceInfo srcInfo {
            .token = ResourceToken {
                .puid = srcBufferState->virtualMapping.puid,
                .type = static_cast<Backend::IL::ResourceTokenType>(srcBufferState->virtualMapping.type),
                .srb= srcBufferState->virtualMapping.srb
            },
            .bufferDescriptor = &srcDescriptor
        };

        // Destination info
        ResourceInfo dstInfo {
            .token = ResourceToken {
                .puid = dstBufferState->virtualMapping.puid,
                .type = static_cast<Backend::IL::ResourceTokenType>(dstBufferState->virtualMapping.type),
                .srb= dstBufferState->virtualMapping.srb
            },
            .bufferDescriptor = &dstDescriptor
        };

        // Invoke hook
        hook.Invoke(context, srcInfo, dstInfo);
    }
}
