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
//! DISPATCH 64, 64, 64

//! SCHEMA "Schemas/Features/Concurrency.h"

//! RESOURCE RWBuffer<R32Float> size:1024
[[vk::binding(0)]] RWBuffer<float> bufferRW : register(u0, space0);

//! RESOURCE RWTexture2DArray<R32Float> size:512,512,512
[[vk::binding(1)]] RWTexture2DArray<float> arrayRW : register(u1, space0);

//! RESOURCE RWTexture2DArray<R32Float> size:32,32,32
[[vk::binding(2)]] RWTexture2DArray<float> arrayRWSmall : register(u2, space0);

[numthreads(4, 4, 4)]
void main(uint3 dtid : SV_DispatchThreadID) {
    // Ensure all addressing is valid
    //! MESSAGE ResourceRaceCondition[0]
    arrayRW[uint3(dtid.x, dtid.y, dtid.z)] = 1.0f;
    
    // Ensure addressing with bounds checking is working
    //! MESSAGE ResourceRaceCondition[0]
    arrayRWSmall[uint3(dtid.x, dtid.y, dtid.z)] = 1.0f;
    
    //! MESSAGE ResourceRaceCondition[>0]
    arrayRW[uint3(dtid.x, dtid.y/2, dtid.z)] = 1.0f;
}
