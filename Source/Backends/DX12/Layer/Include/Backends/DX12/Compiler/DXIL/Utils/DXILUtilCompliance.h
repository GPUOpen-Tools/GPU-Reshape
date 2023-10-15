// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include "Common/Containers/BitArray.h"

/// Compliance utilities
struct DXILUtilCompliance : public DXILUtil {
public:
    DXILUtilCompliance(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table);

    /// Compile this utility
    void Compile();

    /// Trim all unused functions
    void TrimFunctions();

    /// Trim symbols for a given function
    /// \param index function index
    void TrimSymbolForFunction(uint32_t index);

    /// Copy helper
    /// \param out destination
    void CopyTo(DXILUtilCompliance& out);

public:
    /// Mark a declaration as used
    /// \param anchor given declaration
    void MarkAsUsed(const DXILFunctionDeclaration* anchor);

private:
    /// All used declarations
    Vector<const DXILFunctionDeclaration*> declarations;
};
