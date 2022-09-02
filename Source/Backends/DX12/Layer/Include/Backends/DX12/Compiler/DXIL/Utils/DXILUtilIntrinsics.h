#pragma once

// Layer
#include "DXILUtil.h"
#include <Backends/DX12/Compiler/DXIL/Intrinsic/DXILIntrinsicSpec.h>

// Forward declarations
struct DXILFunctionDeclaration;

/// Intrinsic utilities
struct DXILUtilIntrinsics : public DXILUtil {
    using DXILUtil::DXILUtil;

public:
    /// Compile this utility, required before any intrinsic use
    void Compile();

    /// Get an intrinsic
    /// \param spec given intrinsic specification
    /// \return intrinsic function handle
    const DXILFunctionDeclaration* GetIntrinsic(const DXILIntrinsicSpec& spec);

private:
    /// Get an IL type from spec type
    /// \param type spec type
    /// \return IL type
    const Backend::IL::Type* GetType(const DXILIntrinsicTypeSpec& type);

private:
    struct IntrinsicEntry {
        /// Declaration of this intrinsic
        const DXILFunctionDeclaration* declaration{nullptr};
    };

    /// Existing intrinsics
    std::vector<IntrinsicEntry> intrinsics;

    /// Data types
    const Backend::IL::Type *voidType{nullptr};
    const Backend::IL::Type *i1Type{nullptr};
    const Backend::IL::Type *i8Type{nullptr};
    const Backend::IL::Type *i32Type{nullptr};
    const Backend::IL::Type *f32Type{nullptr};
    const Backend::IL::Type *handleType{nullptr};
    const Backend::IL::Type *dimensionsType{nullptr};
};
