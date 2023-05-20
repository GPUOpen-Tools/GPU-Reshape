//! KERNEL   Compute "main"
//! DISPATCH 64, 1, 1

//! SCHEMA "Schemas/Features/Loop.h"

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW : register(u0, space0);

[numthreads(1, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float sum = 0.0f;

    //! MESSAGE LoopTermination[{true}]
    for (uint i = 1; sum != -1.0f; i++) {
        sum += bufferRW[i % 64];
    }

    // No-opt!
    bufferRW[dtid] = sum;
}
