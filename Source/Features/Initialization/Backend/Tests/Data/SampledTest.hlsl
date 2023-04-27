//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

//! SCHEMA "Schemas/Features/Initialization.h"

//! RESOURCE RWTexture2D<R32Float> size:64,64
[[vk::binding(0)]] RWTexture2D<float> textureRW : register(u0, space0);

//! RESOURCE Texture2D<R32Float> size:64,64
[[vk::binding(1)]] Texture2D<float> textureRO : register(t1, space0);

//! RESOURCE StaticSamplerState
[[vk::binding(2)]] SamplerState defaultSampler : register(s2, space0);

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float data = 1.0f;

    //! MESSAGE UninitializedResource[64] LUID:{x == /*Reserved*/ 4 + 1}
    data += textureRO.SampleLevel(defaultSampler, dtid.xx / 64.0f, 0.0f);

    // Store
    textureRW[dtid.xx] = data;
}
