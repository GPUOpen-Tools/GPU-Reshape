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

#include <string_view>
#include <vector>

enum class ResourceType {
    None,
    Buffer,
    RWBuffer,
    StructuredBuffer,
    RWStructuredBuffer,
    Texture1D,
    RWTexture1D,
    Texture2D,
    RWTexture2D,
    RWTexture2DArray,
    Texture3D,
    RWTexture3D,
    SamplerState,
    StaticSamplerState,
    CBuffer
};

struct ResourceInitialization {
    /// Resource sizes (x, [y, [z]])
    std::vector<int64_t> sizes;

    /// Resource data (a, b, c, d...)
    std::vector<int64_t> data;
};

struct Resource {
    /// Type of this resource
    ResourceType type{ResourceType::None};

    /// View and data format
    std::string_view format;

    /// Structured buffer width (0 indicates not a structured buffer)
    uint32_t structuredSize{0};

    /// Optional array size (0 indicates no array)
    uint32_t arraySize{0};

    /// Initialization info
    ResourceInitialization initialization;
};
