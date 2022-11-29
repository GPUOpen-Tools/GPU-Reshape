//! KERNEL   Compute "main"
//! DISPATCH 256, 1, 1
//! DISPATCH 256, 1, 1

//! SCHEMA "Schemas/Features/Descriptor.h"

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW : register(u0, space0);

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(1)]] RWBuffer<float> bufferRW2 : register(u1, space0);

//! RESOURCE Texture2D<R32Float> size:64,64
[[vk::binding(2)]] Texture2D<float> textureRO : register(t2, space0);

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float data = 1.0f;
    data += bufferRW[dtid.x];
    data += textureRO.Load(dtid.x);
	bufferRW2[dtid.x] = data;
}
