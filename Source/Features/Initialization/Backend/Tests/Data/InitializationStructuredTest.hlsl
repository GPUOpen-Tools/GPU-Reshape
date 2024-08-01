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

//! SCHEMA "Schemas/Features/Initialization.h"

struct SubFoo {
    uint dword;
};

struct Foo {
    float dword[4];
    SubFoo subFoo;
};

//! RESOURCE RWStructuredBuffer width:20 size:2048
[[vk::binding(0)]] RWStructuredBuffer<Foo> structuredFooRW : register(u0, space0);

//! RESOURCE RWBuffer<R32Float> size:64
[[vk::binding(1)]] RWBuffer<float> bufferRW : register(u1, space0);

[numthreads(64, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    structuredFooRW[dtid].subFoo.dword = 1.0f;

    // Validate the write chain
    //! MESSAGE UninitializedResource[0]
    bufferRW[dtid * 1] = structuredFooRW[dtid].subFoo.dword;

    // Validate the write chain didn't write unrelated bytes
    //! MESSAGE UninitializedResource[>0]
    bufferRW[dtid * 2] = structuredFooRW[dtid].dword[1];

    // Write all elements
    structuredFooRW[0] = (Foo)0;

    // Validate a full write
    //! MESSAGE UninitializedResource[0]
    bufferRW[dtid * 3] = structuredFooRW[0].dword[0];
}

