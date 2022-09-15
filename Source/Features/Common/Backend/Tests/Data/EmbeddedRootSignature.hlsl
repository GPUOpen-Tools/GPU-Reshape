//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW;

// Root signature
#define RS "RootFlags(0),"  \
           "DescriptorTable(" \
               "UAV(u0, numDescriptors = 4)" \
           ")"

[rootsignature(RS)]
[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    bufferRW[dtid.x] = dtid.x;
}
