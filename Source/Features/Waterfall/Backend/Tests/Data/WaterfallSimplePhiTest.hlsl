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

//! SCHEMA "Schemas/Features/Waterfall.h"

//! RESOURCE CBuffer
[[vk::binding(0)]] cbuffer ConstantBuffer : register(b0, space0) {
    float gScalarData;
}

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(1)]] RWBuffer<float> bufferRW[1] : register(u1, space0);

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(2)]] RWBuffer<float> bufferRWNU[1] : register(u2, space0);

//! RESOURCE RWBuffer<R32UInt> size:64 data:0,1,2,3,4,5,6,7
[[vk::binding(3)]] RWBuffer<uint> bufferRWUI : register(u3, space0);

[numthreads(8, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float varyingArray[10];

    // Populate varying array
    for (int i = 0; i < 10; i++) {
        varyingArray[i] = bufferRW[0][i * dtid];
    }
    
    int phi = 0;
    if (gScalarData) {
        // Force phi instead of select, give it something more expensive to compute
        phi = gScalarData * 33.0f + 0.2f;
    } else {
        phi = 2;
    }

    // Phi instructions are not divergent in and of themselves, only if the CF is divergent
    // In this case, the condition is scalar, therefore convergent

    // Uniform resource indexing
    bufferRW[phi][0] = gScalarData;

    // Uniform VGPR indexing
    bufferRW[phi][dtid * 10 + 0] = varyingArray[phi];

    if (bufferRW[0][dtid]) {
        // Force phi instead of select, give it something more expensive to compute
        phi = gScalarData * 33.0f + 0.2f;
    } else {
        phi = 1;
    }

    // In this case, the condition is a memory read, therefore divergent
    //! MESSAGE WaterfallingCondition[1]
    bufferRW[0][dtid * 10 + 2] = varyingArray[phi];

    // Something that's obviously divergent
    int runtimeDivergent = bufferRWUI[dtid];

    // Validate blatant runtime divergence is checked for
    //! MESSAGE DivergentResourceIndexing[8]
    bufferRW[runtimeDivergent][dtid * 10 + 1] = gScalarData;

    // Validate annotated divergent indexing is checked for
    bufferRWNU[NonUniformResourceIndex(runtimeDivergent)][0] = gScalarData;
}
