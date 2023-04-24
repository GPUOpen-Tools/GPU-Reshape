//! KERNEL    Compute "main"
//! SAFEGUARD
//! EXECUTOR "TextureIndexOOBExecutor"
//! SCHEMA   "Schemas/Features/Descriptor.h"

[[vk::binding(0)]] cbuffer ConstantBuffer : register(b0, space0) {
	uint gCBOffset;
}

[[vk::binding(1)]] RWTexture2D<float> textureRW[] : register(u1, space0);

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    //! MESSAGE DescriptorMismatch[64] isUndefined:0 isOutOfBounds:1
    textureRW[gCBOffset][uint2(dtid.xx)] = gCBOffset;
}
