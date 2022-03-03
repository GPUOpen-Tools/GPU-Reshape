//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

/**
 * Tests structured control flow for selection merges
 */

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW;

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float data = 0.0f;

    // If with selection merge
    if (dtid > 0) {
        data += bufferRW[0];
    }

    GroupMemoryBarrierWithGroupSync();

    // Write back
	bufferRW[dtid.x] = data;
}
