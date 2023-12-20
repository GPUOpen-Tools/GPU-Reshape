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
#include "DXBCPhysicalBlockScan.h"
#include "Blocks/DXBCPhysicalBlockShader.h"
#include "Blocks/DXBCPhysicalBlockPipelineStateValidation.h"
#include "Blocks/DXBCPhysicalBlockRootSignature.h"
#include "Blocks/DXBCPhysicalBlockFeatureInfo.h"
#include "Blocks/DXBCPhysicalBlockInputSignature.h"
#include "Blocks/DXBCPhysicalBlockOutputSignature.h"
#include "Blocks/DXBCPhysicalBlockDebug.h"

// Forward declarations
struct DXParseJob;
struct DXCompileJob;
struct DXStream;
class DXILModule;
class IDXDebugModule;

/// Complete physical block table
struct DXBCPhysicalBlockTable {
    DXBCPhysicalBlockTable(const Allocators &allocators, IL::Program &program);
    ~DXBCPhysicalBlockTable();

    /// Scan the DXBC bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Parse(const DXParseJob& job);

    /// Compile the table
    /// \param job the job to compile against
    /// \return success state
    bool Compile(const DXCompileJob& job);

    /// Stitch the compiled table
    /// \param out destination stream
    bool Stitch(const DXCompileJob& job, DXStream &out, bool sign);

    /// Copy to a new table
    /// \param out the destination table
    void CopyTo(DXBCPhysicalBlockTable& out);

    /// Scanner
    DXBCPhysicalBlockScan scan;

    /// Blocks
    DXBCPhysicalBlockShader shader;
    DXBCPhysicalBlockPipelineStateValidation pipelineStateValidation;
    DXBCPhysicalBlockRootSignature rootSignature;
    DXBCPhysicalBlockFeatureInfo featureInfo;
    DXBCPhysicalBlockInputSignature inputSignature;
    DXBCPhysicalBlockOutputSignature outputSignature;
    DXBCPhysicalBlockDebug debug;

    /// DXBC containers can host DXIL data
    DXILModule* dxilModule{nullptr};

    /// Optional debug data
    IDXDebugModule* debugModule{nullptr};

private:
    /// Is the debugging module owned?
    bool shallowDebug{false};

private:
    Allocators allocators;

    /// IL program
    IL::Program &program;
};
