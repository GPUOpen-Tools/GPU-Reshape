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
//! DISPATCH 4, 1, 1

//! SCHEMA "Schemas/Features/Concurrency.h"

//! RESOURCE RWBuffer<R32Float> size:1024
[[vk::binding(0)]] RWBuffer<float> bufferRW : register(u0, space0);

//! RESOURCE RWStructuredBuffer width:16 size:1024
[[vk::binding(1)]] RWStructuredBuffer<float4> structuredRW : register(u1, space0);

struct SubFoo {
    uint dword;
};

struct Foo {
    float dword[4];
    SubFoo subFoo;
};

//! RESOURCE RWStructuredBuffer width:20 size:1024
[[vk::binding(2)]] RWStructuredBuffer<Foo> structuredFooRW : register(u2, space0);

[numthreads(32, 1, 1)]
void main(uint dtid : SV_DispatchThreadID) {
    //! MESSAGE ResourceRaceCondition[0]
    bufferRW[dtid] = 1.0f;
    
    //! MESSAGE ResourceRaceCondition[0]
    structuredRW[dtid] = 1.0f.xxxx;
    
    //! MESSAGE ResourceRaceCondition[0]
    structuredRW[dtid].xyz = 1.0f.xxx;
    
    //! MESSAGE ResourceRaceCondition[0]
    structuredFooRW[dtid / 4].dword[dtid % 4] = 1.0f;
    
    if (dtid % 2 == 0) {
        //! MESSAGE ResourceRaceCondition[0]
        structuredFooRW[dtid / 2].subFoo.dword = 1.0f;
    }
    
    //! MESSAGE ResourceRaceCondition[>0]
    bufferRW[dtid/2] = 1.0f;
}

