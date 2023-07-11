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
#include "DXBCPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>

struct DXCompileJob;

/// Input signature block
struct DXBCPhysicalBlockOutputSignature : public DXBCPhysicalBlockSection {
    DXBCPhysicalBlockOutputSignature(const Allocators &allocators, Backend::IL::Program &program, DXBCPhysicalBlockTable &table);

    /// Parse signature
    void Parse();

    /// Compile signature
    void Compile();

    /// Compile for compatability
    void CompileDXILCompatability(DXCompileJob& job);

    /// Copy this block
    void CopyTo(DXBCPhysicalBlockOutputSignature &out);

private:
    /// Signature header
    DXILInputSignature header;

    /// Givne element
    struct ElementEntry {
        /// Scanned signature
        DXILSignatureElement scan;

        /// Name string
        const char *semanticName{nullptr};
    };

    /// All entries
    Vector<ElementEntry> entries;
};
