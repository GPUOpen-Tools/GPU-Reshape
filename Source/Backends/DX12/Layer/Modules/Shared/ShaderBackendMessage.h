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
#include "Cxx.h"

/// Token constants
HLSL_CONSTEXPR uint BackendMessageTokenNone            = 0;
HLSL_CONSTEXPR uint BackendMessageTokenScratchOverflow = 1;
HLSL_CONSTEXPR uint BackendMessageAssertion            = 2;

/// Buffer constants
HLSL_CONSTEXPR uint BackendMessageBufferSize       = 1024;
HLSL_CONSTEXPR uint BackendMessageBufferDWordCount = BackendMessageBufferSize / sizeof(uint);

#ifdef __cplusplus
struct BackendMessage {
    uint Token  : 8;
    uint DWords : 24;
};

struct BackendScratchOverflowMessage : public BackendMessage {
    uint RequestedBytes;
};

struct BackendAssertionMessage : public BackendMessage {
    uint DebugDWords[1];
};

static_assert(sizeof(BackendMessage) == sizeof(uint), "Unexpected size");
#else // __cplusplus
template<uint N>
struct DWordArray {
    uint DWords[N];
};

uint PackMessageHeader(uint Token, uint DWords) {
    uint Packed = 0;
    Packed |= Token;
    Packed |= DWords << 8;
    return Packed;
}

void SendScratchOverflowMessage(in RWStructuredBuffer<uint> Out, uint RequestedBytes) {
    uint Head;
    InterlockedAdd(Out[0], 2u, Head);

    // Note: Pre-increment to skip dword count
    Out[++Head] = PackMessageHeader(BackendMessageTokenScratchOverflow, 2u);
    Out[++Head] = RequestedBytes;
}

template<uint N>
void SendAssertionMessage(in RWStructuredBuffer<uint> Out, in DWordArray<N> Args) {
    uint Head;
    InterlockedAdd(Out[0], 1u + N, Head);

    // Note: Pre-increment to skip dword count
    Out[++Head] = PackMessageHeader(BackendMessageAssertion, 1u + N);

    // Write dwords
    for (uint i = 0; i < N; i++) {
        Out[++Head] = Args.DWords[i];
    }
}
#endif // __cplusplus
