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
#include "DXBCPhysicalBlockSection.h"
#include <Backends/DX12/Compiler/DXBC/DXBCHeader.h>

/// Shader block
struct DXBCPhysicalBlockRootSignature : public DXBCPhysicalBlockSection {
    DXBCPhysicalBlockRootSignature(const Allocators &allocators, Backend::IL::Program &program, DXBCPhysicalBlockTable &table);

    /// Parse validation
    void Parse();

    /// Compile validation
    void Compile();

    /// Copy this block
    void CopyTo(DXBCPhysicalBlockRootSignature& out);

private:
    /// Compile the shader export parameters
    void CompileShaderExport();

private:
    struct DescriptorTable {
        DescriptorTable(const Allocators& allocators) : ranges(allocators) {

        }

        /// All ranges within this table
        Vector<DXBCRootSignatureDescriptorRange1> ranges;
    };

    struct RootParameter {
        RootParameter(const Allocators& allocators) : descriptorTable(allocators) {

        }

        /// Type of this parameter
        DXBCRootSignatureParameterType type;

        /// Shader stage visibility
        DXBCRootSignatureVisibility visibility;

        /// Payload data
        DescriptorTable descriptorTable;
        DXBCRootSignatureParameter1 parameter1;
        DXBCRootSignatureConstant constant;
    };

private:
    /// Root signature header
    DXBCRootSignatureHeader header;

    /// All parameters within this root signature
    Vector<RootParameter> parameters;

    /// All samplers within this root signature
    Vector<DXBCRootSignatureSamplerStub> samplers;
};
