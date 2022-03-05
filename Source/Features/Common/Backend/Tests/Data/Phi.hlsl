//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

/**
 * Tests structured control flow for switch merges
 */

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW;

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float value = dtid;

    if (dtid > 30) {
        value = bufferRW[dtid] * 2;
    }

    bufferRW[dtid] = value;
}
