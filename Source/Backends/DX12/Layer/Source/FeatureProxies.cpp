#include <Backends/DX12/FeatureProxies.h>
#include <Backends/DX12/States/CommandListState.h>
#include <Backends/DX12/States/ResourceState.h>
#include <Backends/DX12/Table.Gen.h>

// Backend
#include <Backend/Command/BufferDescriptor.h>

void FeatureHook_CopyBufferRegion::operator()(CommandListState *list, CommandContext *context, ID3D12Resource* pDstBuffer, UINT64 DstOffset, ID3D12Resource* pSrcBuffer, UINT64 SrcOffset, UINT64 NumBytes) const {
    // Get states
    ResourceState* dstState = GetState(pDstBuffer);
    ResourceState* srcState = GetState(pSrcBuffer);

    // Setup source descriptor
    BufferDescriptor srcDescriptor{
        .token = ResourceToken {
            .puid = srcState->virtualMapping.puid,
            .type = static_cast<Backend::IL::ResourceTokenType>(srcState->virtualMapping.type),
            .srb= srcState->virtualMapping.srb
        },
        .offset = SrcOffset,
        .uid = 0u
    };

    // Setup destination descriptor
    BufferDescriptor dstDescriptor{
        .token = ResourceToken {
            .puid = dstState->virtualMapping.puid,
            .type = static_cast<Backend::IL::ResourceTokenType>(dstState->virtualMapping.type),
            .srb= dstState->virtualMapping.srb
        },
        .offset = DstOffset,
        .uid = 0u
    };

    // Invoke hook
    hook.Invoke(context, srcDescriptor, dstDescriptor, NumBytes);
}
