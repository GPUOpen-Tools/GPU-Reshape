//! KERNEL    Compute "main"
//! EXECUTOR "SamplerIndexOOBExecutor"
//! SCHEMA   "Schemas/Features/Descriptor.h"
//! SAFEGUARD

[[vk::binding(0)]] cbuffer ConstantBuffer : register(b0, space0) {
	uint gCBOffset;
}

[[vk::binding(1, 0)]] RWBuffer<float>  bufferRW   : register(u1, space0);
[[vk::binding(2, 0)]] Texture2D<float> textureRO  : register(t2, space0);
[[vk::binding(0, 1)]] SamplerState     samplers[] : register(s0, space1);

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float data = 1.0f;

    //! MESSAGE DescriptorMismatch[0]
    // data += textureRO.SampleLevel(samplers[gCBOffset], 0.0f.xx, 0.0f);

    //! MESSAGE DescriptorMismatch[0]
	bufferRW[dtid.x] = data;
}
