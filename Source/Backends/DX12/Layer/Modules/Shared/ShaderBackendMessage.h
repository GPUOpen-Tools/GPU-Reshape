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

// Shared
#include "Int.h"

#ifdef __cpluscplus
#define HLSL_CONSTEXPR static constexpr
#else // __cpluscplus
#define HLSL_CONSTEXPR static const
#endif // __cpluscplus

/// Token constants
HLSL_CONSTEXPR uint MessageTokenNone            = 0;
HLSL_CONSTEXPR uint MessageTokenScratchOverflow = 1;

/// Buffer constants
HLSL_CONSTEXPR uint BackendMessageBufferSize = 1024;
HLSL_CONSTEXPR uint BackendMessageBufferDWordCount = BackendMessageBufferSize / sizeof(uint);

#ifdef __cplusplus
struct BackendMessage {
    uint Token  : 8;
    uint DWords : 24;
};

struct BackendScratchOverflowMessage : public BackendMessage {
    uint RequestedBytes;
};

static_assert(sizeof(BackendMessage) == sizeof(uint), "Unexpected size");
#else // __cplusplus
uint PackMessageHeader(uint Token, uint DWords) {
    uint Packed = 0;
    Packed |= Token;
    Packed |= DWords << 8;
    return Packed;
}

void SendScratchOverflowMessage(in RWStructuredBuffer<uint> Out, uint RequestedBytes) {
    uint Head;
    InterlockedAdd(Out[0], 2u, Head);

    Out[++Head] = PackMessageHeader(MessageTokenScratchOverflow, 2u);
    Out[++Head] = RequestedBytes;
}
#endif // __cplusplus
