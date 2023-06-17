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

// Std
#include <cstdint>

enum class DXBCPhysicalBlockType : uint32_t {
    /// SM4-5
    Interface = 'EFCI',
    Input = 'NGSI',
    Output5 = '5GSO',
    Output = 'NGSO',
    Patch = 'GSCP',
    Resource = 'FEDR',
    ShaderDebug0 = 'BGDS',
    FeatureInfo = '0IFS',
    Shader4 = 'RDHS',
    Shader5 = 'XEHS',
    ShaderHash = 'HSAH',
    ShaderDebug1 = 'BDPS',
    Statistics = 'TATS',
    PipelineStateValidation = '0VSP',
    RootSignature = '0STR',

    /// SM6
    ILDB = 'BDLI',
    ILDN = 'NDLI',
    DXIL = 'LIXD',
    InputSignature = '1GSI',
    OutputSignature = '1GSO',

    /// Unknown block
    Unexposed = ~0u
};
