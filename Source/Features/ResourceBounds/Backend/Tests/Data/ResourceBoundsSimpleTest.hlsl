// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

//! KERNEL   Compute "main"
//! DISPATCH 4, 1, 1

//! SCHEMA "Schemas/Features/ResourceBounds.h"

//! RESOURCE Buffer<R32Float> size:64
[[vk::binding(0)]] Buffer<float> bufferRO : register(t0, space0);

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(1)]] RWBuffer<float> bufferRW : register(u1, space0);

//! RESOURCE Texture2D<R32Float> size:64,64
[[vk::binding(2)]] Texture2D<float> textureRO : register(t2, space0);

//! RESOURCE RWTexture2D<R32Float> size:64,64
[[vk::binding(3)]] RWTexture2D<float> textureRW : register(u3, space0);

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
