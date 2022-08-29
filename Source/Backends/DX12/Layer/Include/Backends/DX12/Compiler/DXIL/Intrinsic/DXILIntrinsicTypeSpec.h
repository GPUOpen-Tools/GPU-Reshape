#pragma once

enum class DXILIntrinsicTypeSpec {
    Void,

    /// Data types
    I32,
    F32,
    I8,
    I1,

    /// Structured types
    Handle,
    ResRetF32,
    ResRetI32,
};
