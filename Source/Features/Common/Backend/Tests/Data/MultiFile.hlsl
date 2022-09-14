//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

#include "MultiFileCommon.hlsli"

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW;

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float value = dtid;

    if (dtid > 30) {
        value = TransformValue(bufferRW[dtid]);
    }

    GroupMemoryBarrierWithGroupSync();

    bufferRW[dtid] = value;
}
