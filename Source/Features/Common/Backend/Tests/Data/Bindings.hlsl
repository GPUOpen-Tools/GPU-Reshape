//! KERNEL   Compute "main"
//! DISPATCH 1, 1, 1

//! RESOURCE CBuffer
[[vk::binding(0)]] cbuffer ConstantBuffer : register(b0, space0) {
	float gPrefixData;
}

//! RESOURCE Texture2D<RGBA32Float> size:64,64
[[vk::binding(1)]] Texture2D<float4> texture : register(t1, space0);

//! RESOURCE RWBuffer<RGBA32Float> size:64
[[vk::binding(2)]] RWBuffer<float4> bufferRW : register(u2, space0);

//! RESOURCE SamplerState size:64
[[vk::binding(3)]] SamplerState defaultSampler : register(s3, space0);

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float4 data = 0.0f;

    for (uint i = 0; i < 64; i++, bufferRW[dtid] += 1) {
        data += texture.SampleLevel(defaultSampler, dtid.xx / 64.0f, 0.0f);
    }

    GroupMemoryBarrierWithGroupSync();

    // Write back
	bufferRW[dtid.x] = data + gPrefixData;
}
