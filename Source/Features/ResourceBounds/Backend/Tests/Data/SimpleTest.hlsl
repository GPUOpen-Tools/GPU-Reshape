//! KERNEL   Compute "main"
//! DISPATCH 4, 1, 1

//! SCHEMA "Schemas/Features/ResourceBounds.h"

//! RESOURCE Buffer<R32Float> size:64
[[vk::binding(0)]] Buffer<float> bufferRO;

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(1)]] RWBuffer<float> bufferRW;

//! RESOURCE Texture2D<R32Float> size:64,64
[[vk::binding(2)]] Texture2D<float> textureRO;

//! RESOURCE RWTexture2D<R32Float> size:64,64
[[vk::binding(3)]] RWTexture2D<float> textureRW;

[numthreads(1, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
	float noOpt = 0.0f;

    //! CHECK   "Read from buffer 'bufferRW' out of bounds"
    //! MESSAGE ResourceIndexOutOfBounds[3] isTexture:0 isWrite:0
	noOpt += bufferRW[-dtid.x];

    //! CHECK   "Read from texture 'textureRW' out of bounds"
    //! MESSAGE ResourceIndexOutOfBounds[3] isTexture:1 isWrite:0
	noOpt += textureRW[uint2(-dtid.x, 0)];

    //! CHECK   "Write into buffer 'bufferRW' out of bounds"
    //! MESSAGE ResourceIndexOutOfBounds[3] isTexture:0 isWrite:1
	bufferRW[-dtid.x] = noOpt;

    //! CHECK   "Write into texture 'textureRW' out of bounds"
    //! MESSAGE ResourceIndexOutOfBounds[3] isTexture:1 isWrite:1
	textureRW[uint2(-dtid.x, 0)] = noOpt;
}
