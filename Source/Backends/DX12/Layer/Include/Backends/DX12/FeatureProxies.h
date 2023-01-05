#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Backend
#include <Backend/FeatureHook.h>
#include <Backend/FeatureHookTable.h>

// Forward declarations
class CommandListState;

struct FeatureHook_DrawInstanced : TFeatureHook<Hooks::DrawInstanced> {
    void operator()(CommandListState* state, CommandContext* context, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const {
        hook.Invoke(context, vertexCount, instanceCount, firstVertex, firstInstance);
    }
};

struct FeatureHook_DrawIndexedInstanced : TFeatureHook<Hooks::DrawIndexedInstanced> {
    void operator()(CommandListState* state, CommandContext* context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const {
        hook.Invoke(context, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
};

struct FeatureHook_Dispatch : TFeatureHook<Hooks::Dispatch> {
    void operator()(CommandListState* state, CommandContext* context, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) const {
        hook.Invoke(context, threadGroupX, threadGroupY, threadGroupZ);
    }
};

struct FeatureHook_CopyBufferRegion : TFeatureHook<Hooks::CopyBuffer> {
    void operator()(CommandListState* object, CommandContext* context, ID3D12Resource* pDstBuffer, UINT64 DstOffset, ID3D12Resource* pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes) const;
};
