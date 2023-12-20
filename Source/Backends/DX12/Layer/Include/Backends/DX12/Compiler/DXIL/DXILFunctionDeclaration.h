// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMHeader.h>
#include <Backends/DX12/Compiler/DXIL/DXILFunctionSegments.h>

// Backend
#include <Backend/IL/Type.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

// Std
#include <string_view>

struct DXILFunctionDeclaration {
    DXILFunctionDeclaration(const Allocators& allocators) : parameters(allocators), segments(allocators) {
        
    }
    
    /// DXIL anchor of this declaration
    uint64_t anchor;

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

    /// Is this function a prototype?
    bool isPrototype{true};

    /// All parameter values
    TrivialStackVector<uint32_t, 8> parameters;

    /// All segments
    DXILFunctionSegments segments;
};
