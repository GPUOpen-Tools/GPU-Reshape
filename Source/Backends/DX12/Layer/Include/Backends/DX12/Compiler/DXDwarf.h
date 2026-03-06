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
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMHeader.h>

// Backend
#include <Backend/IL/ID.h>

// Common
#include <Common/Containers/TrivialStackVector.h>

namespace Backend::IL {
    struct Type;
}

namespace IL {
    struct Constant;
    struct Function;
}

struct DXDwarfCode {
    /// Owning code offset
    uint32_t codeOffset{IL::InvalidID};
    
    /// Owning constant offset
    const IL::Constant* constant{nullptr};
};

struct DXDwarfValue {
    /// Type of this value
    LLVMDwarfOpKind kind;

    /// Code assigned to this value
    DXDwarfCode code;

    /// Payload
    union {
        struct {
            uint32_t bitStart;
            uint32_t bitLength;
        } bitWise;
    };
};

struct DXDwarfVariableValue {
    /// Name of the variable
    const char* name{nullptr};

    /// Variable being assigned
    uint32_t variableId{0};

    /// Optional, reconstructed type
    const Backend::IL::Type* type{nullptr};

    /// All values assigned
    std::vector<DXDwarfValue> values;
};

struct DXDwarfInfo {
    /// All variables assigned
    std::vector<DXDwarfVariableValue> variables;
};
