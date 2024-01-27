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
[[vk::binding(1)]] RWBuffer<float> bufferRW[] : register(u1, space0);

[numthreads(1, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    int phi = 0;

    if (gScalarData) {
        // Force phi instead of select, give it something more expensive to compute
        phi = gScalarData * 33.0f + 0.2f;
    } else {
        phi = 2;
    }

    // Phi instructions are not divergent in and of themselves, only if the CF is divergent
    // In this case, the condition is scalar, therefore convergent
    bufferRW[phi][0] = gScalarData;

    if (bufferRW[0][0]) {
        // Force phi instead of select, give it something more expensive to compute
        phi = gScalarData * 33.0f + 0.2f;
    } else {
        phi = 1;
    }

    // In this case, the condition is a memory read, therefore divergent
    //-! MESSAGE WaterfallingCondition[1]
    // NOTE: Test disabled until divergence analysis
    bufferRW[phi][0] = gScalarData;

    // Validate annotated divergent indexing is checked for
    bufferRW[NonUniformResourceIndex(phi)][0] = gScalarData;
}
