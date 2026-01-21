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
#endif // __cplusplus

#ifdef __cplusplus
inline uint Low(UInt64 a) {
    return a & 0xFFFFFFFF;
}

inline uint High(UInt64 a) {
    return a >> 32;
}

inline UInt64 Make64(uint low, uint high) {
    return static_cast<UInt64>(low) | (static_cast<UInt64>(high) << 32ull);
}

inline UInt64 AddUInt64_64(UInt64 a, UInt64 b) {
    return a + b;
}

inline UInt64 SubUInt64_64(UInt64 a, UInt64 b) {
    return a - b;
}

inline UInt64 DivUInt64_64_Low(UInt64 a, UInt64 b) {
    return a / b;
}

inline UInt64 MulUInt64_64_Low(UInt64 a, UInt64 b) {
    return a * b;
}
#else // __cplusplus 
uint Low(UInt64 a) {
    return a.x;
}

uint High(UInt64 a) {
    return a.y;
}

UInt64 Make64(uint low, uint high) {
    return UInt64(low, high);
}

UInt64 AddUInt64_64(UInt64 a, UInt64 b) {
    uint low = Low(a) + Low(b);
    return UInt64(low, High(a) + High(b) + ((low < Low(a)) ? 1 : 0));
}

UInt64 AddUInt64_64(UInt64 a, uint b) {
    return AddUInt64_64(a, UInt64(b, 0));
}

UInt64 SubUInt64_64(UInt64 a, UInt64 b) {
    uint low = Low(a) - Low(b);
    return UInt64(low, High(a) - High(b) - ((Low(a) < Low(b)) ? 1 : 0));
}

UInt64 SubUInt64_64(UInt64 a, uint b) {
    return SubUInt64_64(a, UInt64(b, 0));
}

UInt64 DivUInt64_64_Low(UInt64 a, UInt64 b) {
    return UInt64(Low(a) / Low(b), 0);
}

UInt64 MulUInt64_64_Low(UInt64 a, UInt64 b) {
    return UInt64(Low(a) * Low(b), 0);
}

bool LessEqUInt64_64(UInt64 a, UInt64 b) {
    return High(a) < High(b) || (High(a) == High(b) && Low(a) <= Low(b));
}
#endif // __cplusplus
