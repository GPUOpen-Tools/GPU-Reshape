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
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordStringView.h>

// Common
#include <Common/Containers/LinearBlockAllocator.h>

/// Type block
struct DXILPhysicalBlockSymbol : public DXILPhysicalBlockSection {
    DXILPhysicalBlockSymbol(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table);

    /// Copy this block
    /// \param out destination block
    void CopyTo(DXILPhysicalBlockSymbol& out);

public:
    /// Parse all records
    void ParseSymTab(const struct LLVMBlock *block);

    /// Get a value mapping
    /// \param id value id
    /// \return empty if not found
    LLVMRecordStringView GetValueString(uint32_t id);

    /// Get a value cstring mapping, may incur an allocation
    /// \param id value id
    /// \return nullptr if not found
    const char* GetValueAllocation(uint32_t id);

public:
    /// Compile all records
    void CompileSymTab(struct LLVMBlock *block);

    /// Stitch all records
    void StitchSymTab(struct LLVMBlock *block);

private:
    /// Raw values
    Vector<LLVMRecordStringView> valueStrings;

    /// String values, allocated on demand
    Vector<char*> valueAllocations;

    /// Generic allocator
    LinearBlockAllocator<16384> blockAllocator;
};
