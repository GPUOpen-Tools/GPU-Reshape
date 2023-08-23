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

inline const char *GetFormatString(DXGI_FORMAT format) {
    switch (format) {
        default:
            return "UNKNOWN";
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return "R32G32B32A32_TYPELESS";
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return "R32G32B32A32_FLOAT";
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return "R32G32B32A32_UINT";
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return "R32G32B32A32_SINT";
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            return "R32G32B32_TYPELESS";
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return "R32G32B32_FLOAT";
        case DXGI_FORMAT_R32G32B32_UINT:
            return "R32G32B32_UINT";
        case DXGI_FORMAT_R32G32B32_SINT:
            return "R32G32B32_SINT";
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return "R16G16B16A16_TYPELESS";
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return "R16G16B16A16_FLOAT";
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return "R16G16B16A16_UNORM";
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return "R16G16B16A16_UINT";
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return "R16G16B16A16_SNORM";
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return "R16G16B16A16_SINT";
        case DXGI_FORMAT_R32G32_TYPELESS:
            return "R32G32_TYPELESS";
        case DXGI_FORMAT_R32G32_FLOAT:
            return "R32G32_FLOAT";
        case DXGI_FORMAT_R32G32_UINT:
            return "R32G32_UINT";
        case DXGI_FORMAT_R32G32_SINT:
            return "R32G32_SINT";
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            return "R32G8X24_TYPELESS";
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return "D32_FLOAT_S8X24_UINT";
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            return "R32_FLOAT_X8X24_TYPELESS";
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return "X32_TYPELESS_G8X24_UINT";
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            return "R10G10B10A2_TYPELESS";
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return "R10G10B10A2_UNORM";
        case DXGI_FORMAT_R10G10B10A2_UINT:
            return "R10G10B10A2_UINT";
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return "R11G11B10_FLOAT";
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return "R8G8B8A8_TYPELESS";
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return "R8G8B8A8_UNORM";
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return "R8G8B8A8_UNORM_SRGB";
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return "R8G8B8A8_UINT";
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return "R8G8B8A8_SNORM";
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return "R8G8B8A8_SINT";
        case DXGI_FORMAT_R16G16_TYPELESS:
            return "R16G16_TYPELESS";
        case DXGI_FORMAT_R16G16_FLOAT:
            return "R16G16_FLOAT";
        case DXGI_FORMAT_R16G16_UNORM:
            return "R16G16_UNORM";
        case DXGI_FORMAT_R16G16_UINT:
            return "R16G16_UINT";
        case DXGI_FORMAT_R16G16_SNORM:
            return "R16G16_SNORM";
        case DXGI_FORMAT_R16G16_SINT:
            return "R16G16_SINT";
        case DXGI_FORMAT_R32_TYPELESS:
            return "R32_TYPELESS";
        case DXGI_FORMAT_D32_FLOAT:
            return "D32_FLOAT";
        case DXGI_FORMAT_R32_FLOAT:
            return "R32_FLOAT";
        case DXGI_FORMAT_R32_UINT:
            return "R32_UINT";
        case DXGI_FORMAT_R32_SINT:
            return "R32_SINT";
        case DXGI_FORMAT_R24G8_TYPELESS:
            return "R24G8_TYPELESS";
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return "D24_UNORM_S8_UINT";
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            return "R24_UNORM_X8_TYPELESS";
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return "X24_TYPELESS_G8_UINT";
        case DXGI_FORMAT_R8G8_TYPELESS:
            return "R8G8_TYPELESS";
        case DXGI_FORMAT_R8G8_UNORM:
            return "R8G8_UNORM";
        case DXGI_FORMAT_R8G8_UINT:
            return "R8G8_UINT";
        case DXGI_FORMAT_R8G8_SNORM:
            return "R8G8_SNORM";
        case DXGI_FORMAT_R8G8_SINT:
            return "R8G8_SINT";
        case DXGI_FORMAT_R16_TYPELESS:
            return "R16_TYPELESS";
        case DXGI_FORMAT_R16_FLOAT:
            return "R16_FLOAT";
        case DXGI_FORMAT_D16_UNORM:
            return "D16_UNORM";
        case DXGI_FORMAT_R16_UNORM:
            return "R16_UNORM";
        case DXGI_FORMAT_R16_UINT:
            return "R16_UINT";
        case DXGI_FORMAT_R16_SNORM:
            return "R16_SNORM";
        case DXGI_FORMAT_R16_SINT:
            return "R16_SINT";
        case DXGI_FORMAT_R8_TYPELESS:
            return "R8_TYPELESS";
        case DXGI_FORMAT_R8_UNORM:
            return "R8_UNORM";
        case DXGI_FORMAT_R8_UINT:
            return "R8_UINT";
        case DXGI_FORMAT_R8_SNORM:
            return "R8_SNORM";
        case DXGI_FORMAT_R8_SINT:
            return "R8_SINT";
        case DXGI_FORMAT_A8_UNORM:
            return "A8_UNORM";
        case DXGI_FORMAT_R1_UNORM:
            return "R1_UNORM";
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return "R9G9B9E5_SHAREDEXP";
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
            return "R8G8_B8G8_UNORM";
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
            return "G8R8_G8B8_UNORM";
        case DXGI_FORMAT_BC1_TYPELESS:
            return "BC1_TYPELESS";
        case DXGI_FORMAT_BC1_UNORM:
            return "BC1_UNORM";
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return "BC1_UNORM_SRGB";
        case DXGI_FORMAT_BC2_TYPELESS:
            return "BC2_TYPELESS";
        case DXGI_FORMAT_BC2_UNORM:
            return "BC2_UNORM";
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return "BC2_UNORM_SRGB";
        case DXGI_FORMAT_BC3_TYPELESS:
            return "BC3_TYPELESS";
        case DXGI_FORMAT_BC3_UNORM:
            return "BC3_UNORM";
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return "BC3_UNORM_SRGB";
        case DXGI_FORMAT_BC4_TYPELESS:
            return "BC4_TYPELESS";
        case DXGI_FORMAT_BC4_UNORM:
            return "BC4_UNORM";
        case DXGI_FORMAT_BC4_SNORM:
            return "BC4_SNORM";
        case DXGI_FORMAT_BC5_TYPELESS:
            return "BC5_TYPELESS";
        case DXGI_FORMAT_BC5_UNORM:
            return "BC5_UNORM";
        case DXGI_FORMAT_BC5_SNORM:
            return "BC5_SNORM";
        case DXGI_FORMAT_B5G6R5_UNORM:
            return "B5G6R5_UNORM";
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            return "B5G5R5A1_UNORM";
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return "B8G8R8A8_UNORM";
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return "B8G8R8X8_UNORM";
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            return "R10G10B10_XR_BIAS_A2_UNORM";
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return "B8G8R8A8_TYPELESS";
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return "B8G8R8A8_UNORM_SRGB";
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return "B8G8R8X8_TYPELESS";
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return "B8G8R8X8_UNORM_SRGB";
        case DXGI_FORMAT_BC6H_TYPELESS:
            return "BC6H_TYPELESS";
        case DXGI_FORMAT_BC6H_UF16:
            return "BC6H_UF16";
        case DXGI_FORMAT_BC6H_SF16:
            return "BC6H_SF16";
        case DXGI_FORMAT_BC7_TYPELESS:
            return "BC7_TYPELESS";
        case DXGI_FORMAT_BC7_UNORM:
            return "BC7_UNORM";
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return "BC7_UNORM_SRGB";
        case DXGI_FORMAT_AYUV:
            return "AYUV";
        case DXGI_FORMAT_Y410:
            return "Y410";
        case DXGI_FORMAT_Y416:
            return "Y416";
        case DXGI_FORMAT_NV12:
            return "NV12";
        case DXGI_FORMAT_P010:
            return "P010";
        case DXGI_FORMAT_P016:
            return "P016";
        case DXGI_FORMAT_420_OPAQUE:
            return "420_OPAQUE";
        case DXGI_FORMAT_YUY2:
            return "YUY2";
        case DXGI_FORMAT_Y210:
            return "Y210";
        case DXGI_FORMAT_Y216:
            return "Y216";
        case DXGI_FORMAT_NV11:
            return "NV11";
        case DXGI_FORMAT_AI44:
            return "AI44";
        case DXGI_FORMAT_IA44:
            return "IA44";
        case DXGI_FORMAT_P8:
            return "P8";
        case DXGI_FORMAT_A8P8:
            return "A8P8";
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return "B4G4R4A4_UNORM";
        case DXGI_FORMAT_P208:
            return "P208";
        case DXGI_FORMAT_V208:
            return "V208";
        case DXGI_FORMAT_V408:
            return "V408";
        case DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE:
            return "SAMPLER_FEEDBACK_MIN_MIP_OPAQUE";
        case DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE:
            return "SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE";
        case DXGI_FORMAT_FORCE_UINT:
            return "FORCE_UINT";
    }
}

inline uint32_t GetFormatBitSize(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_UNKNOWN:
            return 1u;
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return 128;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return 128;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return 128;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            return 96;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            return 96;
        case DXGI_FORMAT_R32G32B32_UINT:
            return 96;
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return 64;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return 64;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return 64;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return 64;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return 64;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return 64;
        case DXGI_FORMAT_R32G32_TYPELESS:
            return 64;
        case DXGI_FORMAT_R32G32_FLOAT:
            return 64;
        case DXGI_FORMAT_R32G32_UINT:
            return 64;
        case DXGI_FORMAT_R32G32_SINT:
            return 64;
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            return 64;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return 64;
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            return 64;
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return 64;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            return 32;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return 32;
        case DXGI_FORMAT_R10G10B10A2_UINT:
            return 32;
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return 32;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return 32;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return 32;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return 32;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return 32;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return 32;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return 32;
        case DXGI_FORMAT_R16G16_TYPELESS:
            return 32;
        case DXGI_FORMAT_R16G16_FLOAT:
            return 32;
        case DXGI_FORMAT_R16G16_UNORM:
            return 32;
        case DXGI_FORMAT_R16G16_UINT:
            return 32;
        case DXGI_FORMAT_R16G16_SNORM:
            return 32;
        case DXGI_FORMAT_R16G16_SINT:
            return 32;
        case DXGI_FORMAT_R32_TYPELESS:
            return 32;
        case DXGI_FORMAT_D32_FLOAT:
            return 32;
        case DXGI_FORMAT_R32_FLOAT:
            return 32;
        case DXGI_FORMAT_R32_UINT:
            return 32;
        case DXGI_FORMAT_R32_SINT:
            return 32;
        case DXGI_FORMAT_R24G8_TYPELESS:
            return 32;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return 32;
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            return 32;
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return 32;
        case DXGI_FORMAT_R8G8_TYPELESS:
            return 16;
        case DXGI_FORMAT_R8G8_UNORM:
            return 16;
        case DXGI_FORMAT_R8G8_UINT:
            return 16;
        case DXGI_FORMAT_R8G8_SNORM:
            return 16;
        case DXGI_FORMAT_R8G8_SINT:
            return 16;
        case DXGI_FORMAT_R16_TYPELESS:
            return 16;
        case DXGI_FORMAT_R16_FLOAT:
            return 16;
        case DXGI_FORMAT_D16_UNORM:
            return 16;
        case DXGI_FORMAT_R16_UNORM:
            return 16;
        case DXGI_FORMAT_R16_UINT:
            return 16;
        case DXGI_FORMAT_R16_SNORM:
            return 16;
        case DXGI_FORMAT_R16_SINT:
            return 16;
        case DXGI_FORMAT_R8_TYPELESS:
            return 8;
        case DXGI_FORMAT_R8_UNORM:
            return 8;
        case DXGI_FORMAT_R8_UINT:
            return 8;
        case DXGI_FORMAT_R8_SNORM:
            return 8;
        case DXGI_FORMAT_R8_SINT:
            return 8;
        case DXGI_FORMAT_A8_UNORM:
            return 8;
        case DXGI_FORMAT_R1_UNORM:
            return 1;
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return 32;
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
            return 32;
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
            return 32;
        case DXGI_FORMAT_BC1_TYPELESS:
            return 4;
        case DXGI_FORMAT_BC1_UNORM:
            return 4;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            return 4;
        case DXGI_FORMAT_BC2_TYPELESS:
            return 8;
        case DXGI_FORMAT_BC2_UNORM:
            return 8;
        case DXGI_FORMAT_BC2_UNORM_SRGB:
            return 8;
        case DXGI_FORMAT_BC3_TYPELESS:
            return 8;
        case DXGI_FORMAT_BC3_UNORM:
            return 8;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            return 8;
        case DXGI_FORMAT_BC4_TYPELESS:
            return 4;
        case DXGI_FORMAT_BC4_UNORM:
            return 4;
        case DXGI_FORMAT_BC4_SNORM:
            return 4;
        case DXGI_FORMAT_BC5_TYPELESS:
            return 8;
        case DXGI_FORMAT_BC5_UNORM:
            return 8;
        case DXGI_FORMAT_BC5_SNORM:
            return 8;
        case DXGI_FORMAT_B5G6R5_UNORM:
            return 16;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            return 16;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return 32;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return 32;
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            return 32;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return 32;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return 32;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return 32;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return 32;
        case DXGI_FORMAT_BC6H_TYPELESS:
            return 8;
        case DXGI_FORMAT_BC6H_UF16:
            return 8;
        case DXGI_FORMAT_BC6H_SF16:
            return 8;
        case DXGI_FORMAT_BC7_TYPELESS:
            return 8;
        case DXGI_FORMAT_BC7_UNORM:
            return 8;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 8;
        case DXGI_FORMAT_AYUV:
            return 32;
        case DXGI_FORMAT_Y410:
            return 32;
        case DXGI_FORMAT_Y416:
            return 64;
        case DXGI_FORMAT_NV12:
            return 12;
        case DXGI_FORMAT_P010:
            return 24;
        case DXGI_FORMAT_P016:
            return 24;
        case DXGI_FORMAT_420_OPAQUE:
            return 12;
        case DXGI_FORMAT_YUY2:
            return 32;
        case DXGI_FORMAT_Y210:
            return 64;
        case DXGI_FORMAT_Y216:
            return 64;
        case DXGI_FORMAT_NV11:
            return 12;
        case DXGI_FORMAT_AI44:
            return 8;
        case DXGI_FORMAT_IA44:
            return 8;
        case DXGI_FORMAT_P8:
            return 8;
        case DXGI_FORMAT_A8P8:
            return 16;
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 16;
        case DXGI_FORMAT_P208:
            return 16;
        case DXGI_FORMAT_V208:
            return 16;
        case DXGI_FORMAT_V408:
            return 24;
        default:
            return 0;
    }
}
