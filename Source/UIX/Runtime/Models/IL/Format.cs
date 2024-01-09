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

namespace Studio.Models.IL
{
    /// <summary>
    /// Mirror of Cxx Format, keep up to date
    /// </summary>
    public enum Format
    {
        None,
        RGBA32Float,
        RGBA16Float,
        R32Float,
        R32Snorm,
        R32Unorm,
        RGBA8,
        RGBA8Snorm,
        RG32Float,
        RG16Float,
        R11G11B10Float,
        R16Float,
        RGBA16,
        RGB10A2,
        RG16,
        RG8,
        R16,
        R8,
        RGBA16Snorm,
        RG16Snorm,
        RG8Snorm,
        R16Snorm,
        R16Unorm,
        R8Snorm,
        RGBA32Int,
        RGBA16Int,
        RGBA8Int,
        R32Int,
        RG32Int,
        RG16Int,
        RG8Int,
        R16Int,
        R8Int,
        RGBA32UInt,
        RGBA16UInt,
        RGBA8UInt,
        R32UInt,
        RGB10a2UInt,
        RG32UInt,
        RG16UInt,
        RG8UInt,
        R16UInt,
        R8UInt,
        Unexposed
    }
}