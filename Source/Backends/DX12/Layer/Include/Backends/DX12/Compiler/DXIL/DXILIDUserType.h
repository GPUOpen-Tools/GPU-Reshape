#pragma once

enum class DXILIDUserType {
    /// Scalar value
    Singular,

    /// Vector from structured type (member0, ... memberN)
    VectorOnStruct,

    /// Vector from sequential value indices (LLVMValueX, ... LLVMValueX+N)
    VectorOnSequential,
};
