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

#pragma once

#ifdef __cplusplus
#include <cstdint>

using uint   = uint32_t;
using uint3  = uint32_t[3];
using UInt64 = uint64_t;
#else // __cplusplus 
using UInt64 = uint2;

uint High(UInt64 a) {
    return a.x;
}

uint Low(UInt64 a) {
    return a.y;
}

UInt64 AddUInt64_64(UInt64 a, UInt64 b) {
    uint low = Low(a) + Low(b);
    return UInt64(High(a) + High(b) + ((Low(a) < Low(b)) ? 1 : 0), low);
}

UInt64 SubUInt64_64(UInt64 a, UInt64 b) {
    uint low = Low(a) - Low(b);
    return UInt64(High(a) - High(b) - ((Low(a) < Low(b)) ? 1 : 0), low);
}
#endif // __cplusplus
