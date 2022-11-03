#pragma once

// Backend
#include <Backend/FeatureHook.h>
#include <Backend/FeatureHookTable.h>

struct FeatureHook_vkCmdDrawIndexed : TFeatureHook<Hooks::DrawIndexedInstanced> {
    void operator()(CommandContext* context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const {
        hook.Invoke(context, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
};
