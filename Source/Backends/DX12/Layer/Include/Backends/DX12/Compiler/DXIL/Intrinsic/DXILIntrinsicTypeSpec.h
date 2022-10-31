#pragma once

enum class DXILIntrinsicTypeSpec {
    Void,

    /// Data types
    F64,
    F32,
    F16,
    I64,
    I32,
    I8,
    I1,

    /// Structured types
    Handle,
    Dimensions,
    ResRetF32,
    ResRetI32,
    CBufRetF32,
    CBufRetI32,
};
