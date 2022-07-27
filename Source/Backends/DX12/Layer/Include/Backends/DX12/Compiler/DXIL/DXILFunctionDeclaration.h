#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMHeader.h>

// Backend
#include <Backend/IL/Type.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

// Std
#include <string_View>

struct DXILFunctionDeclaration {
    /// DXIL identifier of this declaration
    uint64_t id;

    /// Name of this declaration
    std::string_view name;

    /// Type of this declaration
    const Backend::IL::FunctionType* type{nullptr};

    /// Hash of name
    size_t hash{0};

    /// Associated linkage
    LLVMLinkage linkage;

    /// All parameter values
    TrivialStackVector<uint32_t, 8> parameters;
};
