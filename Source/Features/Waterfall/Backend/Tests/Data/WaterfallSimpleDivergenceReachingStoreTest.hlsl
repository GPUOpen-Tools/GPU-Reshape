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

//! RESOURCE CBuffer data:0
[[vk::binding(0)]] cbuffer ConstantBuffer : register(b0, space0) {
    uint gScalarA;
    uint gScalarB;
    uint gScalarArray[10];
}

//! RESOURCE RWBuffer<R32UInt> size:64
[[vk::binding(1)]] RWBuffer<uint> bufferRW[] : register(u1, space0);

[numthreads(1, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    uint value = 0;

    // Populate varying array
    float varyingArray[10];
    for (int i = 0; i < 10; i++) {
        varyingArray[i] = bufferRW[0][i * dtid];
    }
    
    // Value terminator
    value = gScalarArray[bufferRW[0][0]];

    // Test that reads from uniform memory with divergent indices are detected
    //! MESSAGE WaterfallingCondition[1]
    bufferRW[0][dtid * 10 + 4] = varyingArray[value];

    // Value terminator
    value = 0.0f;
    
    // Uniform
    if (gScalarA) {
        value = 1;
    }

    // Test that uniform branching is fine
    bufferRW[value][dtid * 10 + 0] = value;

    // Value terminator
    value = 0.0f;

    // Uniform
    if (gScalarA) {
        value = 1;
    }

    // Uniform
    if (gScalarA) {
        value = 1;
    }

    // Test that multiple uniform branching is fine
    bufferRW[value][dtid * 10 + 1] = value;

    // Value terminator
    value = 0;

    // Uniform
    if (gScalarA) {
        value = 1;
    }

    // Divergent
    if (bufferRW[0][dtid]) {
        value = 1;
    }

    // Test that divergent stores are detected
    //! MESSAGE WaterfallingCondition[1]
    bufferRW[0][dtid * 10 + 2] = varyingArray[value];

    // Value terminator
    value = 0;

    // Uniform, but divergent value
    if (gScalarA) {
        value = bufferRW[0][0];
    }

    // Uniform
    if (gScalarB) {
        value = 1;
    }

    // Test that uniform stores but divergent values are detected
    //! MESSAGE WaterfallingCondition[1]
    bufferRW[0][dtid * 10 + 3] = varyingArray[value];

    // Value terminator
    value = 0;

    // Diverging on structured data
    uint4 selection = bufferRW[0][0].xxxx;
    if (selection[0]) {
        value = 1;
    }

    // Test that structured addressing is successfully propagated
    //! MESSAGE WaterfallingCondition[1]
    bufferRW[0][dtid * 10 + 4] = varyingArray[value];
}
