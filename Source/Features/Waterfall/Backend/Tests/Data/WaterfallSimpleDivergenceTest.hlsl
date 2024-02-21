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
    uint gScalarIndex;
    uint gScalarArray[10];
}

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(1)]] RWBuffer<float> bufferRW : register(u1, space0);

[numthreads(1, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float varyingArray[10];

    // Populate varying array
    for (int i = 0; i < 10; i++) {
        varyingArray[i] = i * dtid;
    }

    // Divergence values
    uint uniformValue   = gScalarArray[gScalarIndex];
    uint divergentValue = bufferRW[dtid];

    // Simple check, uniform indexing into constant array
    bufferRW[dtid * 10 + 0] = varyingArray[uniformValue];

    // Simple check, validate divergent values
    //! MESSAGE WaterfallingCondition[1]
    bufferRW[dtid * 10 + 1] = varyingArray[divergentValue];

    // Simple check, validate scalarization of divergent values works
    bufferRW[dtid * 10 + 2] = varyingArray[WaveReadLaneFirst(divergentValue)];

}
