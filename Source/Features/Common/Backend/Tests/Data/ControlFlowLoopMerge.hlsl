//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

/**
 * Tests structured control flow for loop merges
 */

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW;

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float data = 0.0f;

    // For with loop merge (compiled in debug)
    for (uint i = 0; i < 64; i++) {
        data += bufferRW[dtid.x];
    }

    GroupMemoryBarrierWithGroupSync();

    // Write back
	bufferRW[dtid.x] = data;
}