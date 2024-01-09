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
#include <Backends/DX12/Compiler/DXIL/DXILFunctionDeclaration.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordReader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordStringView.h>
#include "DXILPhysicalBlockSection.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

/// Function block
struct DXILPhysicalBlockFunctionAttribute : public DXILPhysicalBlockSection {
    DXILPhysicalBlockFunctionAttribute(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table);

    /// Copy this block
    /// \param out destination block
    void CopyTo(DXILPhysicalBlockFunctionAttribute& out);

    /// Set the declaration block
    /// \param block program block
    void SetDeclarationBlock(struct LLVMBlock *block);

public:
    /// Parse a function
    /// \param block source block
    void ParseParameterAttributeGroup(struct LLVMBlock *block);

    /// Parse a function
    /// \param block source block
    void ParseParameterBlock(struct LLVMBlock *block);

public:
    /// Get an attribute list
    /// \param count the number of attributes
    /// \param values all attribute values
    /// \return index
    uint32_t FindOrCompileAttributeList(uint32_t count, const LLVMParameterGroupValue* values);

private:
    struct ParameterAttribute {
        /// Parameter index
        uint32_t idx{~0u};

        /// Givne value
        LLVMParameterGroupValue value{LLVMParameterGroupValue::None};

        /// Optional string view
        LLVMRecordStringView view;
    };

    struct ParameterAttributeGroup {
        TrivialStackVector<ParameterAttribute, 16> attributes;
    };

    struct ParameterGroup {
        TrivialStackVector<ParameterAttribute, 16> attributes;
    };

    /// All attribute groups
    Vector<ParameterAttributeGroup> parameterAttributeGroups;

    /// All parameter groups
    Vector<ParameterGroup> parameterGroups;

private:
    /// Declaration blocks
    LLVMBlock* groupDeclarationBlock{nullptr};
    LLVMBlock* parameterDeclarationBlock{nullptr};
};
