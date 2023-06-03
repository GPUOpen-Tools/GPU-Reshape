//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

//! RESOURCE RWBuffer<R32UInt> size:64
[[vk::binding(0)]] RWBuffer<uint> bufferRW;

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID) {
    bufferRW[dtid.x] = dtid.x;
}
