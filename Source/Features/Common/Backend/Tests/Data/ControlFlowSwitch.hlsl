//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

/**
 * Tests structured control flow for switch merges
 */

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW;

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    switch (dtid.x) {
        default:
            break;
        case 0:
            bufferRW[0] = 0;
            break;
        case 1:
            bufferRW[1] = 0;
            break;
        case 2:
            bufferRW[2] = 0;
            break;
    }
}
