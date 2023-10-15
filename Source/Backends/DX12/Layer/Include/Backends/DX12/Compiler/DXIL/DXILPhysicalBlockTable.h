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
#include "DXILPhysicalBlockScan.h"
#include "Blocks/DXILPhysicalBlockFunction.h"
#include "Blocks/DXILPhysicalBlockType.h"
#include "Blocks/DXILPhysicalBlockGlobal.h"
#include "Blocks/DXILPhysicalBlockString.h"
#include "Blocks/DXILPhysicalBlockMetadata.h"
#include "Blocks/DXILPhysicalBlockSymbol.h"
#include "Blocks/DXILPhysicalBlockFunctionAttribute.h"
#include "Utils/DXILUtilIntrinsics.h"
#include "Utils/DXILUtilCompliance.h"
#include "DXILIDMap.h"
#include "DXILIDRemapper.h"
#include "DXILBindingInfo.h"

// Common
#include <Common/Containers/LinearBlockAllocator.h>

// Std
#include <string>

// Forward declarations
struct DXCompileJob;
struct DXStream;

struct DXILPhysicalBlockTable {
    DXILPhysicalBlockTable(const Allocators &allocators, Backend::IL::Program &program);

    /// Parse the DXIL bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Parse(const void* byteCode, uint64_t byteLength);

    /// Compile the table
    /// \param job the job to compile against
    /// \return success state
    bool Compile(const DXCompileJob& job);

    /// Stitch the compiled table
    /// \param out destination stream
    void Stitch(DXStream &out);

    /// Copy to a new table
    /// \param out the destination table
    void CopyTo(DXILPhysicalBlockTable& out);

    /// Scanner
    DXILPhysicalBlockScan scan;

    /// Blocks
    DXILPhysicalBlockFunction function;
    DXILPhysicalBlockFunctionAttribute functionAttribute;
    DXILPhysicalBlockType type;
    DXILPhysicalBlockGlobal global;
    DXILPhysicalBlockString string;
    DXILPhysicalBlockSymbol symbol;
    DXILPhysicalBlockMetadata metadata;

    /// Utilities
    DXILUtilIntrinsics intrinsics;
    DXILUtilCompliance compliance;

    /// Shared binding info
    DXILBindingInfo bindingInfo;

    /// IL program
    IL::Program& program;

    /// Shared identifier map
    DXILIDMap idMap;

    /// Shared identifier remapper
    DXILIDRemapper idRemapper;

    /// LLVM triple
    std::string triple;

    /// LLVM data layout
    std::string dataLayout;

public:
    /// Shared allocator for records
    LinearBlockAllocator<sizeof(uint64_t) * 1024u> recordAllocator;
};
