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

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(0)]] RWBuffer<float> bufferRW : register(u0, space0);

// Random global data
static const float3 globalConstant[] = {
	float3(1, 0, 0),
	float3(0, 1, 0),
	float3(0, 0, 1)
};

[numthreads(8, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    float constantA = fmod(3.0f, 2.0f) + cos(1);
    float constantB = constantA + 3.0f;
    float varyingC = constantB + dtid;

    // Array tests
    float constantArray[10];
    float varyingArray[10];

    // Populate compile time array
    // This test is compiled under debug, no constant-folding will take place
    for (int i = 0; i < 10; i++) {
        constantArray[i] = i;
    }

    // Populate varying array
    for (int i = 0; i < 10; i++) {
        varyingArray[i] = i * dtid;
    }

    // Simple check, static indexing into constant array
    bufferRW[dtid * 50 + 0] = constantArray[0];
    bufferRW[dtid * 50 + 1] = constantArray[constantA];
    bufferRW[dtid * 50 + 2] = constantArray[constantB];

    // Simple check, dynamic indexing into constant array
    bufferRW[dtid * 50 + 3] = constantArray[dtid % 10];
    bufferRW[dtid * 50 + 4] = constantArray[varyingC];

    // Simple check, static indexing into varying array
    bufferRW[dtid * 50 + 5] = varyingArray[0];
    bufferRW[dtid * 50 + 6] = varyingArray[constantA];
    bufferRW[dtid * 50 + 7] = varyingArray[constantB];

    // Simple check, varying indexing into varying array
    //! MESSAGE WaterfallingCondition[1]
    bufferRW[dtid * 50 + 8] = varyingArray[dtid % 10];
    //! MESSAGE WaterfallingCondition[1]
    bufferRW[dtid * 50 + 9] = varyingArray[varyingC];

	// Simple check, static indexing into global constant array
	bufferRW[dtid * 50 + 10] = globalConstant[constantB % 3].x;

	// Simple check, dynamic indexing into global constant array
	bufferRW[dtid * 50 + 11] = globalConstant[dtid % 2].x;
}
