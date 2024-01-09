// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
//! DISPATCH 1, 1, 1

//! RESOURCE CBuffer
[[vk::binding(0)]] cbuffer ConstantBuffer : register(b0, space0) {
	float gPrefixData;
}

//! RESOURCE Texture2D<RGBA32Float> size:64,64
[[vk::binding(1)]] Texture2D<float4> texture : register(t1, space0);

//! RESOURCE RWTexture2D<RGBA32Float> size:64,64
[[vk::binding(2)]] RWTexture2D<float4> textureRW : register(u2, space0);

//! RESOURCE RWBuffer<RGBA32Float> size:64
[[vk::binding(3)]] RWBuffer<float4> bufferRW : register(u3, space0);

//! RESOURCE StaticSamplerState size:64
[[vk::binding(4)]] SamplerState defaultSampler : register(s4, space0);

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float4 data = 0.0f;

    for (uint i = 0; i < 64; i++, bufferRW[dtid] += 1) {
        data += texture.SampleLevel(defaultSampler, dtid.xx / 64.0f, 0.0f);
    }

    for (uint j = 0; j < 64; j++, bufferRW[dtid] += 1) {
        data += texture.Load(uint3(dtid.xx, 0));
    }

    GroupMemoryBarrierWithGroupSync();

    // Write back
    bufferRW[dtid.x] = data + gPrefixData;
    textureRW[dtid.xx] = data + gPrefixData;
}
