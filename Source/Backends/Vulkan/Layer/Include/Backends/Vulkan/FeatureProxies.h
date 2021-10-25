#pragma once

// Layer
#include "FeatureHook.h"

// Backend
#include <Backend/FeatureHookTable.h>

struct FeatureHook_vkCmdDrawIndexed : TFeatureHook<Hooks::DrawIndexed> {
    void operator()(VkCommandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const {
        hook.Invoke(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
};
