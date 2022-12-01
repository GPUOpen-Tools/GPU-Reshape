//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

//! SCHEMA "Schemas/Features/Initialization.h"

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW : register(u0, space0);

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(1)]] RWBuffer<float> bufferRW2 : register(u1, space0);

//! RESOURCE Texture2D<R32Float> size:64,64
[[vk::binding(2)]] Texture2D<float> textureRO : register(t2, space0);

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float data = 1.0f;

    //! MESSAGE UninitializedResource[0]
    data += bufferRW[dtid.x];

    //! MESSAGE UninitializedResource[0]
    data += textureRO.Load(dtid.x);

    //! MESSAGE UninitializedResource[0]
	bufferRW2[dtid.x] = data;
}
