// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

/**
 * Tests group shared data
 */

//! RESOURCE RWBuffer<R32UInt> size:64
[[vk::binding(0)]] RWBuffer<uint> bufferRW;

groupshared uint GSData[64];
groupshared uint GSSumData;

[numthreads(64, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID) {
    if (dtid.x == 0) {
        GSSumData = 0;
        GSData[dtid.x] = dtid.y + dtid.z;
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint i = dtid.x; i < 64; i++) {
        uint _;
        InterlockedAdd(GSSumData, bufferRW[i] + GSData[dtid.x], _);
    }

    GroupMemoryBarrierWithGroupSync();

    bufferRW[dtid.x] += GSSumData;
}
