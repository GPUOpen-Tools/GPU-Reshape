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
/// SBT's are just an array of shader identifiers followed by user specific data.
///   ---------------------
///   | Shader Identifier |
///   ---------------------
///   |        -          |
///   |    User Data      |
///   |        -          |
///   ---------------------
///
/// We embed the patch index into the identifier.
///   ---------------------
///   | Shader Identifier |
///   ---------------------
///   |  Patching Index   |
///   ---------------------
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

/// The in-place patched dwords
RWBuffer<uint> RWSBTPatchedDWords : register(u1);

/// The patched identifier dwords
Buffer<uint> PatchIdentifierDWords : register(t2);

SBTShaderGroupIdentifierEmbeddedData GetEmbeddedData(uint Offset) {
    SBTShaderGroupIdentifierEmbeddedData Data;
    Data.PatchIndex = RWSBTPatchedDWords[Offset + Constants.NativeIdentifierDWordStride + 0];
    return Data;
}

[numthreads(32, 1, 1)]
void main(uint ShaderRecordIndex : SV_DispatchThreadID) {
    if (ShaderRecordIndex >= Constants.SBTRecordCount) {
        return;
    }

    // Current identifier offset
    uint DWordOffset = ShaderRecordIndex * Constants.IdentifierDWordStride;

    // Get the embedded data after native
    SBTShaderGroupIdentifierEmbeddedData Embedded = GetEmbeddedData(DWordOffset);

    // Patch identifier offset
    uint PatchDWordOffset = Embedded.PatchIndex * Constants.PatchIdentifierDWordStride;

    // Copy the patched shader identifier over
    // (May not actually be instrumented)
    for (uint DWordIndex = 0; DWordIndex < Constants.NativeIdentifierDWordStride; DWordIndex++) {
        RWSBTPatchedDWords[DWordOffset + DWordIndex] = PatchIdentifierDWords[PatchDWordOffset + DWordIndex];
    }
}
