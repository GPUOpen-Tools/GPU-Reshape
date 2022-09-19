//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

/**
 * Tests group shared data
 */

//! RESOURCE RWBuffer<R32UInt> size:64
[[vk::binding(0)]] RWBuffer<uint> bufferRW;

groupshared uint GSData[64];
groupshared uint GSSumData;

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID) {
    if (dtid.x == 0) {
        GSSumData = 0;
        GSData[dtid.x] = dtid.y + dtid.z;
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint i = dtid.x; i < 64; i++) {
        uint _;
        InterlockedAdd(GSSumData, bufferRW[i] + GSData[dtid.x], _);
    }

    GroupMemoryBarrierWithGroupSync();

    bufferRW[dtid.x] += GSSumData;
}
