#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/Intrinsic/DXILIntrinsicTypeSpec.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMHeader.h>

// Std
#include <cstdint>
#include <string_view>
#include <vector>

struct DXILIntrinsicSpec {
    /// Unique identifier of this intrinsic
    uint32_t uid{~0u};

    /// RST name
    std::string_view name;

    /// Intrinsic return type
    DXILIntrinsicTypeSpec returnType;

    /// Intrinsic parameter type
    std::vector<DXILIntrinsicTypeSpec> parameterTypes;

    /// Function parameter group
    std::vector<LLVMParameterGroupValue> parameterGroup;
};
