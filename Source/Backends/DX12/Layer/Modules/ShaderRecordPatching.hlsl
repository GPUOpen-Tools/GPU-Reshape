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

#include "Shared/ShaderRecordPatching.h"
#include "Shared/ShaderBackendMessage.h"

///
/// SBT's are just an array of shader identifiers followed by the local root signature specific user data.
/// ---------------------
/// | Shader Identifier |
/// ---------------------
/// |        -          |
/// |    User Data      |
/// |        -          |
/// ---------------------
/// 
/// We extend all user data with custom data, populated by this shader, which is then used for PRMT indexing
///  +++++++++++++++++++
/// |  Descriptor Data  |
/// ---------------------
///
/// Since we are unable to "embed" metadata into the shader identifier, we need to perform a runtime lookup
/// from the 8 dword to a single index, specific to the instrumented pipeline.
/// ---------------------
/// | Shader Identifier |  ->  Patching Index
/// ---------------------
///
/// Each instrumented pipeline, the entire state object, generates a new patching table
/// indexed by the mapping above.
///     --------------------
/// 0:  | Patch Identifier |
///     --------------------
/// 1:  | Patch Identifier |
///     --------------------
/// 2:  | Patch Identifier |
///     --------------------
///
/// The original shader identifier is then replaced by the patched identifier.
/// 

/// All constant data
ConstantBuffer<SBTPatchConstantData> Constants : register(b0);

/// Effectively a hash map, but points to a start+end pair
/// No probing nor checking required
Buffer<uint2> SBTIdentifierTable : register(t1);

/// Indexed by the hash map indices
StructuredBuffer<SBTIdentifierTableEntry> SBTIdentifierList : register(t2);

/// The linear patch identifiers
StructuredBuffer<SBTIdentifierPatch> SBTPatchedIdentifiers : register(t3);

/// The user / source shader binding table
StructuredBuffer<uint> SBTSourceDWords : register(t4);

/// The patched shader binding table
RWStructuredBuffer<uint> RWSBTPatchedDWords : register(u5);

/// The PRM descriptor data buffer
RWStructuredBuffer<uint> RWDescriptorData : register(u6);

/// Backend messages for diagnostics
RWStructuredBuffer<uint> RWBackendMessageBuffer : register(u7);

/// Check if a dword is set in the addressing masks
bool IsSet(in uint Masks[2], uint dword) {
    return (Masks[dword / 32] & (1u << (dword % 32))) != 0x0;
}

/// Read a source identifier
/// \param Offset dword offset
/// \return identifier
SBTIdentifier GetSourceIdentifier(uint Offset) {
    SBTIdentifier Out;

    [unroll]
    for (uint i = 0; i < 8; i++) {
        Out.DWords[i] = SBTSourceDWords[Offset + i];
    }
    
    return Out;
}

/// Find an identifier entry
/// \param Identifier identifier to lookup
/// \return entry
SBTIdentifierTableEntry GetShaderIdentifierIndex(in SBTIdentifier Identifier) {
    uint Hash = ShaderIdentifierHash(Identifier);

    // Mod by table length
    uint Offset = Hash % Constants.SBTIdentifierTableSize;

    // Effectively the known-good search range
    uint2 StartAndEnd = SBTIdentifierTable[Offset];

    for (uint i = StartAndEnd.x; i < StartAndEnd.y; i++) {
        // Note that entries with different hashes may share the same location, since we mod it
        if (SBTIdentifierList[i].Matches(Hash, Identifier)) {
            return SBTIdentifierList[i];
        }
    }

    // If we couldn't match the index, it may be that the application is not using this entry
#if 0
    DWordArray<11> Data = {
        Hash, StartAndEnd.x, StartAndEnd.y,
        Identifier.DWords[0], Identifier.DWords[1], Identifier.DWords[2], Identifier.DWords[3],
        Identifier.DWords[4], Identifier.DWords[5], Identifier.DWords[6], Identifier.DWords[7]
    };
    
    SendAssertionMessage(RWBackendMessageBuffer, Data);
#endif // 0
    
    // Just point it to the first one
    return (SBTIdentifierTableEntry)0;
}

uint GetAlignedVAddrDWord(uint DWord) {
    // VAddr's need to be aligned to 64 bits, which is two dwords
    return (DWord + 1) & ~1;
}

[numthreads(32, 1, 1)]
void main(uint ShaderRecordIndex : SV_DispatchThreadID) {
    if (ShaderRecordIndex >= Constants.SBTRecordCount) {
        return;
    }
    
    // Dword offsets
    uint SourceDWordOffset   = ShaderRecordIndex * Constants.SourceDWordStride;
    uint PatchedDWordOffset  = ShaderRecordIndex * Constants.PatchedDWordStride;

    // Descriptor PRM offsets
    uint DescriptorWriteStart  = ShaderRecordIndex * Constants.DescriptorConstantStride;

    // Hash lookup of the shader record
    SBTIdentifierTableEntry IdentifierEntry = GetShaderIdentifierIndex(GetSourceIdentifier(SourceDWordOffset));
    SBTIdentifierPatch      IdentifierPatch = SBTPatchedIdentifiers[IdentifierEntry.Index];
    
    // Copy the patched shader identifier over
    // (May not actually be instrumented)
    for (uint SIDWordIndex = 0; SIDWordIndex < 8; SIDWordIndex++) {
        RWSBTPatchedDWords[PatchedDWordOffset + SIDWordIndex] = IdentifierPatch.Patch.DWords[SIDWordIndex];
    }

    // Skip the shader identifier
    PatchedDWordOffset += 8;
    SourceDWordOffset  += 8;

    // Current iteration offsets
    uint SourceParameterDWordOffset  = SourceDWordOffset;
    uint PatchedParameterDWordOffset = PatchedDWordOffset;

    // Patch all dwords, SBT's within a wave will likely share the same root signature, so it should
    // be somewhat coherent, hopefully.
    for (uint RootParameterIndex = 0; RootParameterIndex < IdentifierEntry.ParameterCount; RootParameterIndex++) {
        SBTRootParameterTypeInfo Parameter = IdentifierEntry.SBTSourceParameters[RootParameterIndex];

        // Handle type
        switch (Parameter.GetType()) {
            default: {
                DWordArray<1> Data = { __LINE__ };
                SendAssertionMessage(RWBackendMessageBuffer, Data);
                break;
            }
            case SBTRootParameterType::VAddr64:
            case SBTRootParameterType::InlinePRM: {
                UInt64 VAddr;
                VAddr.x = SBTSourceDWords[SourceParameterDWordOffset++];
                VAddr.y = SBTSourceDWords[SourceParameterDWordOffset++];

                // Determine physical resource mapping offset, (VAddr - Base) / Stride
                // With the current descriptor limits, we can just assume the low part after subtracting base
                uint PRMOffset;
                if (Parameter.GetVAddr64HeapIndex() == SamplerHeapIndex) {
                    PRMOffset = Low(SubUInt64_64(VAddr, Constants.SamplerHeapOffset)) / Constants.SamplerHeapStride;
                } else {
                    PRMOffset = Low(SubUInt64_64(VAddr, Constants.ResourceHeapOffset)) / Constants.ResourceHeapStride;
                }

                // Inline handling is different
                if (Parameter.GetType() == SBTRootParameterType::VAddr64) {
                    // Write the offset linearly
                    RWDescriptorData[Parameter.GetPRMTOffset()] = PRMOffset;
                } else {
                    DWordArray<1> Data = { __LINE__ };
                    SendAssertionMessage(RWBackendMessageBuffer, Data);
                    
                    // TODO[rt]: Actually fetch the PRMT and write it inline based on the PRMOffset
                    [unroll]
                    for (uint i = 0; i < SBTInlineTokenMetadatDWordCount; i++) {
                        RWDescriptorData[Parameter.GetPRMTOffset() + i] = 0;
                    }
                }
                
                RWSBTPatchedDWords[PatchedParameterDWordOffset++] = Low(VAddr);
                RWSBTPatchedDWords[PatchedParameterDWordOffset++] = High(VAddr);
                break;
            }
            case SBTRootParameterType::Constant: {
                // Just copy over the dwords
                for (uint DWordIndex = 0; DWordIndex < Parameter.GetConstantDWordCount(); DWordIndex++) {
                    uint DWord = SBTSourceDWords[SourceParameterDWordOffset++];
                    RWSBTPatchedDWords[PatchedParameterDWordOffset++] = DWord;
                }
                break;
            }
        }
    }

    // Get the aligned start address, must be aligned to two dwords
    uint AlignedDescriptorSBTStart = GetAlignedVAddrDWord(IdentifierEntry.SBTDWords);

    // Write the descriptor address to the patched SBT
    UInt64 DescriptorVAddr = AddUInt64_64(Constants.DescriptorConstantStart, UInt64(DescriptorWriteStart * 4, 0));
    RWSBTPatchedDWords[PatchedDWordOffset + AlignedDescriptorSBTStart + 0] = Low(DescriptorVAddr);
    RWSBTPatchedDWords[PatchedDWordOffset + AlignedDescriptorSBTStart + 1] = High(DescriptorVAddr);
}
