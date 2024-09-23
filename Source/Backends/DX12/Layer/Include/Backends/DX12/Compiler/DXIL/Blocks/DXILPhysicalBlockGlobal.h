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
#include "DXILPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXIL/DXILConstantMap.h>

/// Type block
struct DXILPhysicalBlockGlobal : public DXILPhysicalBlockSection {
    DXILPhysicalBlockGlobal(const Allocators &allocators, IL::Program &program, DXILPhysicalBlockTable &table);

    /// Copy this block
    /// \param out destination block
    void CopyTo(DXILPhysicalBlockGlobal& out);

public:
    /// Parse a constant block
    /// \param block source block
    void ParseConstants(struct LLVMBlock* block);

    /// Parse a global variable
    /// \param record source record
    void ParseGlobalVar(struct LLVMRecord& record);

    /// Parse an alias
    /// \param record source record
    void ParseAlias(struct LLVMRecord& record);

    /// Resolve all global variables
    /// Maps optional forward constants
    void ResolveGlobals();

public:
    /// Compile a constant block
    /// \param block block
    void CompileConstants(struct LLVMBlock* block);

    /// Compile a global variable
    /// \param record record
    void CompileGlobalVar(struct LLVMRecord& record);

    /// Compile an alias
    /// \param record record
    void CompileAlias(struct LLVMRecord& record);

public:
    /// Stitch a constant block
    /// \param block block
    void StitchConstants(struct LLVMBlock* block);

    /// Stitch a global variable
    /// \param record record
    void StitchGlobalVar(struct LLVMRecord& record);

    /// Stitch an alias
    /// \param record record
    void StitchAlias(struct LLVMRecord& record);

public:
    /// Underlying constants
    DXILConstantMap constantMap;

private:
    /// All pending resolves
    Vector<std::pair<IL::Variable*, IL::ID>> initializerResolves;
};
