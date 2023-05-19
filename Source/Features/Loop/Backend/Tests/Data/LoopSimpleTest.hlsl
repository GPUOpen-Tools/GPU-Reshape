//! KERNEL   Compute "main"
//! DISPATCH 64, 1, 1

//! SCHEMA "Schemas/Features/Loop.h"

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW : register(u0, space0);

[numthreads(1, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    bufferRW[dtid] = 1.0f;

    float sum = 0.0f;

    // ! MESSAGE LoopTermination[>0]
    for (uint i = 1; sum != -1.0f; i++) {
        sum += max(0, bufferRW[i % 64]);
    }
}
