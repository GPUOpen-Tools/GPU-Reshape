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

// Backend
#include <Backend/IL/ResourceTokenType.h>
#include <Backend/IL/Format.h>

// Std
#include <cstdint>

/// Unpacked token type
struct ResourceToken {
    /// Physical UID of the resource
    uint32_t puid;

    /// Type of the resource
    Backend::IL::ResourceTokenType type;

    /// Format of the resource
    Backend::IL::Format format;

    /// Size of the format
    uint32_t formatSize;
    
    /// Width of the resource
    uint32_t width{1};
    
    /// Height of the resource
    uint32_t height{1};
    
    /// Depth or the number of slices in the resource
    uint32_t depthOrSliceCount{1};

    /// Mip count of the resource
    uint32_t mipCount{1};
    
    /// Base mip level of the resource
    uint32_t baseMip{0};

    /// Base slice of the resource
    uint32_t baseSlice{0};
};
