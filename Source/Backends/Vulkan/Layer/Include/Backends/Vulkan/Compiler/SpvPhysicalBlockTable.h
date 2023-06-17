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
#include "Blocks/SpvPhysicalBlockCapability.h"
#include "Blocks/SpvPhysicalBlockDebugStringSource.h"
#include "Blocks/SpvPhysicalBlockAnnotation.h"
#include "Blocks/SpvPhysicalBlockTypeConstantVariable.h"
#include "Blocks/SpvPhysicalBlockFunction.h"
#include "Blocks/SpvPhysicalBlockEntryPoint.h"
#include "Utils/SpvUtilShaderExport.h"
#include "Utils/SpvUtilShaderPRMT.h"
#include "Utils/SpvUtilShaderDescriptorConstantData.h"
#include "Utils/SpvUtilShaderConstantData.h"

/// Combined physical block table
struct SpvPhysicalBlockTable {
    SpvPhysicalBlockTable(const Allocators &allocators, Backend::IL::Program &program);

    /// Parse a stream
    /// \param code stream start
    /// \param count stream word count
    /// \return success state
    bool Parse(const uint32_t *code, uint32_t count);

    /// Compile the table
    /// \param job the job to compile against
    /// \return success state
    bool Compile(const SpvJob& job);

    /// Stitch the compiled table
    /// \param out destination stream
    void Stitch(SpvStream &out);

    /// Copy to a new table
    /// \param out the destination table
    void CopyTo(SpvPhysicalBlockTable& out);

    IL::Program& program;

    /// Scanner
    SpvPhysicalBlockScan scan;

    /// Physical blocks
    SpvPhysicalBlockEntryPoint entryPoint;
    SpvPhysicalBlockCapability capability;
    SpvPhysicalBlockAnnotation annotation;
    SpvPhysicalBlockDebugStringSource debugStringSource;
    SpvPhysicalBlockTypeConstantVariable typeConstantVariable;
    SpvPhysicalBlockFunction function;

    /// Utilities
    SpvUtilShaderExport shaderExport;
    SpvUtilShaderPRMT shaderPRMT;
    SpvUtilShaderDescriptorConstantData shaderDescriptorConstantData;
    SpvUtilShaderConstantData shaderConstantData;
};