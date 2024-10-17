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
[[vk::binding(0)]] RWBuffer<uint> bufferRW : register(u0, space0);

[numthreads(8, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    uint array[10];

    // Populate varying array
    for (int i = 0; i < 10; i++) {
        array[i] = bufferRW[dtid + i];
    }

    uint varyingData = bufferRW[dtid];
    
    //! MESSAGE WaterfallingCondition[1]
    bufferRW[dtid * 10] = array[varyingData];

    // Standard loop
    for (int i = 0; i < 4; i++) {
        // Uniform read in loop
        bufferRW[dtid * 10 + i] = array[i];

        // Uniform read in divergent control flow
        if (bufferRW[dtid * 20]) {
            bufferRW[dtid * 10 + i] = array[i];
        }
    }

    // Conditional termination
    for (int i = 0; i < 4; ++i) {
        if (bufferRW[dtid * 20]) {
            bufferRW[dtid * 10 + i] = array[i];
            break;
        }
    }

    // Unconditional termination
    for (int i = 0; i < 4; i++) {
        bufferRW[dtid * 10 + i] = array[i];
        break;
    }

    // Validate that we're checking for edges and instructions that haven't been executed
    bool definitelyFalse = 1 + 1 == 1;
    if (definitelyFalse) {
        for (int i = 0; i < 4; i++) {
            bufferRW[dtid * 10 + i] = array[i];
        }
    }
}
