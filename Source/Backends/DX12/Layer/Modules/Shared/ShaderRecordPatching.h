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

struct SBTPatchConstantData {
    uint ResourceHeapStride;
    uint SamplerHeapStride;
    uint SourceDWordStride;
    uint PatchedDWordStride;
    
    uint DescriptorConstantStride;
    uint SBTIdentifierTableSize;
    uint SBTRecordCount;
    uint Pad0;

    UInt64 ResourceHeapOffset;
    UInt64 SamplerHeapOffset;
    UInt64 DescriptorConstantStart;
    UInt64 Pad1;

    uint CBAlignPadding[64-16];
};

struct SBTIdentifier {
    /// All opaque identifier dwords
    uint DWords[8];
};

/// Hash a shader identifier
#ifdef __cplusplus
inline uint ShaderIdentifierHash(const SBTIdentifier& ID) {
#else // __cplusplus
uint ShaderIdentifierHash(in SBTIdentifier ID) {
#endif // __cplusplus
    uint Hash = 0;

#ifndef __cplusplus
    [unroll]
#endif // __cplusplus
    for (uint i = 0; i < 8; i++) {
        uint Key = ID.DWords[i];

        // Murmur
        Key ^= Key >> 16;
        Key *= 0x85ebca6b;
        Key ^= Key >> 13;
        Key *= 0xc2b2ae35;
        Key ^= Key >> 16;

        // Combine
        Hash ^= Key;
        Hash *= 0x9e3779b1;
    }

    return Hash;
}

enum class SBTRootParameterType {
    VAddr64   = 0,
    InlinePRM = 1,
    Constant  = 2
};

HLSL_CONSTEXPR uint ResourceHeapIndex = 0;
HLSL_CONSTEXPR uint SamplerHeapIndex  = 1;

struct SBTRootParameterTypeInfo {
    // 0:2   Type
    // 2:17  PRMT-Offset
    // 17:32 Payload
    //   VAddr
    //   17:17  Heap Index
    //   Constant
    //   17:32 DWord Count
#if __cplusplus
    uint type : 2;
    uint prmtOffset : 15;
    uint payload : 15;
#else // __cplusplus
    uint Packed;

    SBTRootParameterType GetType() {
        return (SBTRootParameterType)(Packed & 0b11);
    }

    uint GetPRMTOffset() {
        return (Packed >> 2) & 0x7FFF;
    }

    uint GetConstantDWordCount() {
        return Packed >> 17;
    }

    uint GetVAddr64HeapIndex() {
        return (Packed >> 17) & 0b1;
    }
#endif // __cplusplus
};

struct SBTIdentifierTableEntry {
#ifndef __cplusplus
    /// Check if a given entry matches an identifier
    inline bool Matches(uint OtherKey, in SBTIdentifier OtherIdentifier) {
        if (Key != OtherKey) {
            return false;
        }

        // Double check identifier, hash collisions are expected
        [unroll]
        for (uint i = 0; i < 8; i++) {
            if (Identifier.DWords[i] != OtherIdentifier.DWords[i]) {
                return false;
            }
        }

        return true;
    }
#endif // __cplusplus

    /// Header
    uint Key;
    uint Index;
    uint SBTDWords;
    uint ParameterCount;

    /// Identifier data
    SBTIdentifier Identifier;

    /// Local root signature addressing data
    SBTRootParameterTypeInfo SBTSourceParameters[64];
};

struct SBTIdentifierPatch {
    /// Header
    uint  Metadata;
    uint3 Padding;

    /// Patched shader identifier
    SBTIdentifier Patch;
};

/// Is this table entry set?
static const uint SBTIdentifierTableFlagSet = 1;

/// Is the identifier patch actually instrumented?
static const uint SBTIdentifierPatchFlagInstrumented = 1;
