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

// Common
#include <Common/Assert.h>

namespace Backend::IL {
    enum class TextureDimension {
        Texture1D,
        Texture2D,
        Texture3D,
        Texture1DArray,
        Texture2DArray,
        Texture2DCube,
        Texture2DCubeArray,
        SubPass,
        Unexposed,
    };

    /// Get the dimension size / count of the mdoe
    inline uint32_t GetDimensionSize(TextureDimension dim) {
        switch (dim) {
            default:
                ASSERT(false, "Invalid dimension");
                return 0;
            case TextureDimension::SubPass:
                ASSERT(false, "Sub-pass has no inherit dimensionality");
                return 0;
            case TextureDimension::Texture1D:
                return 1;
            case TextureDimension::Texture2D:
                return 2;
            case TextureDimension::Texture3D:
                return 3;
            case TextureDimension::Texture1DArray:
                return 2;
            case TextureDimension::Texture2DArray:
                return 3;
            case TextureDimension::Texture2DCube:
                return 3;
            case TextureDimension::Texture2DCubeArray:
                return 4;
        }
    }
}
