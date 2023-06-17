// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
    const Backend::IL::Type *resRetI32{nullptr};
    const Backend::IL::Type *resRetF32{nullptr};
    const Backend::IL::Type *cbufRetI32{nullptr};
    const Backend::IL::Type *cbufRetF32{nullptr};
};
