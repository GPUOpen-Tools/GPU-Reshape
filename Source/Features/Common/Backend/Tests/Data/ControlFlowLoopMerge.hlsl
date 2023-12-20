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
 * Tests structured control flow for loop merges
 */

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW;

float DoFilter(inout float data, uint dtid) {
    // Test selection constructs in loops
    for (uint x = 0; x < 64; x++) {
        if (x % (x / 2) == 0) {
            continue;
        }

        data += bufferRW[dtid + x];
    }

    // Test selection constructs in inner-loops
    for (uint x = 0; x < 64; x++) {
        for (uint y = 0; y < 64; y++) {
            if (x % (x / 2) == 0) {
                continue;
            }

            data += bufferRW[dtid + x * y];
        }
    }

    // Test selection constructs in outer-loops
    for (uint x = 0; x < 64; x++) {
        if (x % (x / 2) == 0) {
            continue;
        }

        for (uint y = 0; y < 64; y++) {
            data += bufferRW[dtid + x * y];
        }
    }

    return data;
}

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float data = 0.0f;

    // For with loop merge (compiled in debug)
    for (uint i = 0; i < 64; i++) {
        data += bufferRW[dtid.x];
    }

    [unroll]
    for (uint k = 0; k < 64; k++) {
        data += bufferRW[dtid.x * 3u];
    }

    if (dtid % 128 == 0) {
        return;
    }

    GroupMemoryBarrierWithGroupSync();

    // Test potential splits in continue block
    for (uint j = 0; j < 64; j++, bufferRW[dtid] += 1) {
        data += bufferRW[dtid.x];
    }

    GroupMemoryBarrierWithGroupSync();

    if (dtid > 64) {
        return;
    }

    data = DoFilter(data, dtid);

    GroupMemoryBarrierWithGroupSync();

    // Write back
	bufferRW[dtid.x] = data;
}
