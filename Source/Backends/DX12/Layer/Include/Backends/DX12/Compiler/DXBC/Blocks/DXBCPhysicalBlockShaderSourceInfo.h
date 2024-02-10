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
#include <Backends/DX12/Compiler/DXBC/DXBCPhysicalBlockScan.h>

/// Shader source block
struct DXBCPhysicalBlockShaderSourceInfo {
    DXBCPhysicalBlockShaderSourceInfo(DXBCPhysicalBlockScan& scan);

    /// Parse source info
    void Parse();

public:
    struct SourceArg {
        /// Name of the argument
        std::string_view name;

        /// Optional, assigned value of the argument
        std::string_view value;
    };

    struct SourceFile {
        /// Path of the file
        std::string_view filename;

        /// Contents of the file
        std::string_view contents;
    };

    /// All compilation arguments
    std::vector<SourceArg> sourceArgs;

    /// All source files
    std::vector<SourceFile> sourceFiles;

private:
    /// Optional, storage for zlib decompression
    std::vector<uint8_t> decompressionBlob;

    /// PDB block scanner
    DXBCPhysicalBlockScan& scan;
};
