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

// Backend
#include <Backend/IL/Format.h>

// System
#include <d3d12.h>

inline DXGI_FORMAT Translate(Backend::IL::Format format) {
    switch (format) {
        default:
            ASSERT(false, "Unexpected format");
            return {};
        case Backend::IL::Format::RGBA32Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Backend::IL::Format::RGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case Backend::IL::Format::R32Float:
            return DXGI_FORMAT_R32_FLOAT;
        case Backend::IL::Format::RGBA8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Backend::IL::Format::RGBA8Snorm:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case Backend::IL::Format::RG32Float:
            return DXGI_FORMAT_R32G32_FLOAT;
        case Backend::IL::Format::RG16Float:
            return DXGI_FORMAT_R16G16_FLOAT;
        case Backend::IL::Format::R11G11B10Float:
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case Backend::IL::Format::R16Float:
            return DXGI_FORMAT_R16_FLOAT;
        case Backend::IL::Format::RGBA16:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case Backend::IL::Format::RGB10A2:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case Backend::IL::Format::RG16:
            return DXGI_FORMAT_R16G16_UNORM;
        case Backend::IL::Format::RG8:
            return DXGI_FORMAT_R8G8_UNORM;
        case Backend::IL::Format::R16:
            return DXGI_FORMAT_R16_UNORM;
        case Backend::IL::Format::R8:
            return DXGI_FORMAT_R8_UNORM;
        case Backend::IL::Format::RGBA16Snorm:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case Backend::IL::Format::RG16Snorm:
            return DXGI_FORMAT_R16G16_SNORM;
        case Backend::IL::Format::RG8Snorm:
            return DXGI_FORMAT_R8G8_SNORM;
        case Backend::IL::Format::R16Snorm:
            return DXGI_FORMAT_R16_SNORM;
        case Backend::IL::Format::R8Snorm:
            return DXGI_FORMAT_R8_SNORM;
        case Backend::IL::Format::RGBA32Int:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case Backend::IL::Format::RGBA16Int:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case Backend::IL::Format::RGBA8Int:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case Backend::IL::Format::R32Int:
            return DXGI_FORMAT_R32_SINT;
        case Backend::IL::Format::RG32Int:
            return DXGI_FORMAT_R32G32_SINT;
        case Backend::IL::Format::RG16Int:
            return DXGI_FORMAT_R16G16_SINT;
        case Backend::IL::Format::RG8Int:
            return DXGI_FORMAT_R8G8_SINT;
        case Backend::IL::Format::R16Int:
            return DXGI_FORMAT_R16_SINT;
        case Backend::IL::Format::R8Int:
            return DXGI_FORMAT_R8_SINT;
        case Backend::IL::Format::RGBA32UInt:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case Backend::IL::Format::RGBA16UInt:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case Backend::IL::Format::RGBA8UInt:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case Backend::IL::Format::R32UInt:
            return DXGI_FORMAT_R32_UINT;
        case Backend::IL::Format::RGB10a2UInt:
            return DXGI_FORMAT_R10G10B10A2_UINT;
        case Backend::IL::Format::RG32UInt:
            return DXGI_FORMAT_R32G32_UINT;
        case Backend::IL::Format::RG16UInt:
            return DXGI_FORMAT_R16G16_UINT;
        case Backend::IL::Format::RG8UInt:
            return DXGI_FORMAT_R8G8_UINT;
        case Backend::IL::Format::R16UInt:
            return DXGI_FORMAT_R16_UINT;
        case Backend::IL::Format::R8UInt:
            return DXGI_FORMAT_R8_UINT;
    }
}
