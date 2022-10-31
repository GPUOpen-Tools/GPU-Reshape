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

    static uint32_t kInbuiltCount = 3;
}
