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

/// The user / source shader binding table
Buffer<uint> SBTSourceDWords : register(t1);

/// The patched shader binding table
RWBuffer<uint> RWSBTPatchedDWords : register(u2);

/// The PRM descriptor data buffer
RWBuffer<uint> RWDescriptorData : register(u3);

/// Effectively a hash map, but points to a start+end pair
/// No probing nor checking required
Buffer<uint2> SBTIdentifierTable : register(t4);

/// Indexed by the hash map indices
StructuredBuffer<SBTIdentifierTableEntry> SBTIdentifierList : register(t5);

/// The linear patch identifiers
StructuredBuffer<SBTIdentifierPatch> SBTPatchedIdentifiers : register(t6);

/// Check if a dword is set in the addressing masks
bool IsSet(in uint Masks[2], uint dword) {
    return (Masks[dword / 32] & (1 << (dword % 32))) != 0x0;
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
    
    // Unreachable,... theoretically!
    return (SBTIdentifierTableEntry)0;
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

    // Local address indexing
    uint VAddrIndex = 0;

    // Iterate over dwords
    // We don't actually modify the source dwords (and their vaddr's), but we do write the PRM's
    for (uint DWordIndex = 0; DWordIndex < IdentifierEntry.SBTDWords; DWordIndex++) {
        // Copy dword to patched
        uint DWordLow = SBTSourceDWords[SourceDWordOffset + DWordIndex];
        RWSBTPatchedDWords[PatchedDWordOffset + DWordIndex] = DWordLow;

        // Not a VAddr? Skip it, including the low parts.
        // TODO[rt]: I don't think this makes sense with root typed srvs...
        if (!IsSet(IdentifierEntry.SBTSourceDWordVAddrBitmasks, DWordIndex)) {
            continue;
        }

        UInt64 VAddr = UInt64(DWordLow, SBTSourceDWords[SourceDWordOffset + DWordIndex + 1]);

        // Determine physical resource mapping offset, (VAddr - Base) / Stride
        // With the current descriptor limits, we can just assume the low part after subtracting base
        uint PRMOffset;
        if (IsSet(IdentifierEntry.SBTSourceDWordSamplerBitmasks, DWordIndex)) {
            PRMOffset = Low(SubUInt64_64(VAddr, Constants.SamplerHeapOffset)) / Constants.SamplerHeapStride;
        } else {
            PRMOffset = Low(SubUInt64_64(VAddr, Constants.ResourceHeapOffset)) / Constants.ResourceHeapStride;
        }

        // Write the offset linearly
        uint DescriptorDWordOffset = IdentifierEntry.SBTSourceDWordOffsets[VAddrIndex++];
        RWDescriptorData[DescriptorWriteStart + DescriptorDWordOffset] = PRMOffset;
    }

    // Write the descriptor address to the patched SBT
    UInt64 DescriptorVAddr = AddUInt64_64(Constants.DescriptorConstantStart, UInt64(DescriptorWriteStart * 4, 0));
    RWSBTPatchedDWords[PatchedDWordOffset + IdentifierEntry.SBTDWords + 0] = Low(DescriptorVAddr);
    RWSBTPatchedDWords[PatchedDWordOffset + IdentifierEntry.SBTDWords + 1] = High(DescriptorVAddr);
}
