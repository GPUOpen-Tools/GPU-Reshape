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
#include "DXBCPhysicalBlockSection.h"

// Std
#include <string_view>

// Forward declarations
struct DXParseJob;

/// Debug block
struct DXBCPhysicalBlockDebug : public DXBCPhysicalBlockSection {
    DXBCPhysicalBlockDebug(const Allocators &allocators, Backend::IL::Program &program, DXBCPhysicalBlockTable &table);

    /// Parse all instructions
    bool Parse(const DXParseJob& job);

private:
    /// Try to parse a PDB file
    /// \param path given path
    /// \return nullptr if failed
    DXBCPhysicalBlock* TryParsePDB(const std::string_view& path);

private:
    /// Scanner for external pdbs
    DXBCPhysicalBlockScan pdbScanner;

    /// Tie lifetime of external pdb to this block
    Vector<uint8_t> pdbContainerContents;
};
