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

// Layer
#include <Backends/DX12/Compiler/DXIL/Intrinsic/DXILIntrinsicSpec.h>

namespace Intrinsics {
    static DXILIntrinsicSpec DxOpIsSpecialFloatF32 {
        .uid = 0,
        .name = "dx.op.isSpecialFloat.f32",
        .returnType = DXILIntrinsicTypeSpec::I1,
        .parameterTypes = {
            DXILIntrinsicTypeSpec::I32, // opcode
            DXILIntrinsicTypeSpec::F32, // value
        }
    };

    static DXILIntrinsicSpec DxOpIsSpecialFloatF16 {
        .uid = 1,
        .name = "dx.op.isSpecialFloat.f16",
        .returnType = DXILIntrinsicTypeSpec::I1,
        .parameterTypes = {
            DXILIntrinsicTypeSpec::I32, // opcode
            DXILIntrinsicTypeSpec::F16, // value
        }
    };

    static DXILIntrinsicSpec DxOpCBufferLoadLegacyI32 {
        .uid = 2,
        .name = "dx.op.cbufferLoadLegacy.i32",
        .returnType = DXILIntrinsicTypeSpec::CBufRetI32,
        .parameterTypes = {
            DXILIntrinsicTypeSpec::I32, // opcode
            DXILIntrinsicTypeSpec::Handle, // resource handle
            DXILIntrinsicTypeSpec::I32 // 0-based row index (row = 16-byte DXBC register)
        }
    };
    
    static DXILIntrinsicSpec DxOpWaveActiveAllEqualI32 {
        .uid = 3,
        .name = "dx.op.waveActiveAllEqual.i32",
        .returnType = DXILIntrinsicTypeSpec::I1,
        .parameterTypes = {
            DXILIntrinsicTypeSpec::I32, // opcode
            DXILIntrinsicTypeSpec::I32, // value
        }
    };

    static DXILIntrinsicSpec DxOpCreateHandleFromBinding {
        .uid = 4,
        .name = "dx.op.createHandleFromBinding",
        .returnType = DXILIntrinsicTypeSpec::Handle,
        .parameterTypes = {
            DXILIntrinsicTypeSpec::I32, // opcode
            DXILIntrinsicTypeSpec::ResBind, // binding
            DXILIntrinsicTypeSpec::I32, // range index
            DXILIntrinsicTypeSpec::I1 // non-uniform
        }
    };

    static DXILIntrinsicSpec DxOpAnnotateHandle {
        .uid = 5,
        .name = "dx.op.annotateHandle",
        .returnType = DXILIntrinsicTypeSpec::Handle,
        .parameterTypes = {
            DXILIntrinsicTypeSpec::I32, // opcode
            DXILIntrinsicTypeSpec::Handle, // resource handle
            DXILIntrinsicTypeSpec::ResourceProperties, // properties
        }
    };

    static uint32_t kInbuiltCount = 6;
}
