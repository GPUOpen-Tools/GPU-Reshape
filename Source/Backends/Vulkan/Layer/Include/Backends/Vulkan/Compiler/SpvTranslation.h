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
#include "Spv.h"

// Common
#include <Common/Assert.h>

// Backend
#include <Backend/IL/Format.h>
#include <Backend/IL/AddressSpace.h>
#include <Backend/IL/TextureDimension.h>

inline SpvImageFormat Translate(Backend::IL::Format format) {
    switch (format) {
        default:
            ASSERT(false, "Invalid format");
            return SpvImageFormatUnknown;
        case Backend::IL::Format::RGBA32Float:
            return SpvImageFormat::SpvImageFormatRgba32f;
        case Backend::IL::Format::RGBA16Float:
            return SpvImageFormat::SpvImageFormatRgba16f;
        case Backend::IL::Format::R32Float:
            return SpvImageFormat::SpvImageFormatR32f;
        case Backend::IL::Format::RGBA8:
            return SpvImageFormat::SpvImageFormatRgba8;
        case Backend::IL::Format::RGBA8Snorm:
            return SpvImageFormat::SpvImageFormatRgba8Snorm;
        case Backend::IL::Format::RG32Float:
            return SpvImageFormat::SpvImageFormatRg32f;
        case Backend::IL::Format::RG16Float:
            return SpvImageFormat::SpvImageFormatRg16f;
        case Backend::IL::Format::R11G11B10Float:
            return SpvImageFormat::SpvImageFormatR11fG11fB10f;
        case Backend::IL::Format::R16Float:
            return SpvImageFormat::SpvImageFormatR16f;
        case Backend::IL::Format::RGBA16:
            return SpvImageFormat::SpvImageFormatRgba16;
        case Backend::IL::Format::RGB10A2:
            return SpvImageFormat::SpvImageFormatRgb10A2;
        case Backend::IL::Format::RG16:
            return SpvImageFormat::SpvImageFormatRg16;
        case Backend::IL::Format::RG8:
            return SpvImageFormat::SpvImageFormatRg8;
        case Backend::IL::Format::R16:
            return SpvImageFormat::SpvImageFormatR16;
        case Backend::IL::Format::R8:
            return SpvImageFormat::SpvImageFormatR8;
        case Backend::IL::Format::RGBA16Snorm:
            return SpvImageFormat::SpvImageFormatRgba16Snorm;
        case Backend::IL::Format::RG16Snorm:
            return SpvImageFormat::SpvImageFormatRg16Snorm;
        case Backend::IL::Format::RG8Snorm:
            return SpvImageFormat::SpvImageFormatRg8Snorm;
        case Backend::IL::Format::R16Snorm:
            return SpvImageFormat::SpvImageFormatR16Snorm;
        case Backend::IL::Format::R8Snorm:
            return SpvImageFormat::SpvImageFormatR8Snorm;
        case Backend::IL::Format::RGBA32Int:
            return SpvImageFormat::SpvImageFormatRgba32i;
        case Backend::IL::Format::RGBA16Int:
            return SpvImageFormat::SpvImageFormatRgba16i;
        case Backend::IL::Format::RGBA8Int:
            return SpvImageFormat::SpvImageFormatRgba8i;
        case Backend::IL::Format::R32Int:
            return SpvImageFormat::SpvImageFormatR32i;
        case Backend::IL::Format::RG32Int:
            return SpvImageFormat::SpvImageFormatRg32i;
        case Backend::IL::Format::RG16Int:
            return SpvImageFormat::SpvImageFormatRg16i;
        case Backend::IL::Format::RG8Int:
            return SpvImageFormat::SpvImageFormatRg8i;
        case Backend::IL::Format::R16Int:
            return SpvImageFormat::SpvImageFormatR16i;
        case Backend::IL::Format::R8Int:
            return SpvImageFormat::SpvImageFormatR8i;
        case Backend::IL::Format::RGBA32UInt:
            return SpvImageFormat::SpvImageFormatRgba32ui;
        case Backend::IL::Format::RGBA16UInt:
            return SpvImageFormat::SpvImageFormatRgba16ui;
        case Backend::IL::Format::RGBA8UInt:
            return SpvImageFormat::SpvImageFormatRgba8ui;
        case Backend::IL::Format::R32UInt:
            return SpvImageFormat::SpvImageFormatR32ui;
        case Backend::IL::Format::RGB10a2UInt:
            return SpvImageFormat::SpvImageFormatRgb10a2ui;
        case Backend::IL::Format::RG32UInt:
            return SpvImageFormat::SpvImageFormatRg32ui;
        case Backend::IL::Format::RG16UInt:
            return SpvImageFormat::SpvImageFormatRg16ui;
        case Backend::IL::Format::RG8UInt:
            return SpvImageFormat::SpvImageFormatRg8ui;
        case Backend::IL::Format::R16UInt:
            return SpvImageFormat::SpvImageFormatR16ui;
        case Backend::IL::Format::R8UInt:
            return SpvImageFormat::SpvImageFormatR8ui;
    }
}

inline Backend::IL::Format Translate(SpvImageFormat format) {
    switch (format) {
        default:
            return Backend::IL::Format::Unexposed;
        case SpvImageFormatUnknown:
            return Backend::IL::Format::None;
        case SpvImageFormatRgba32f:
            return Backend::IL::Format::RGBA32Float;
        case SpvImageFormatRgba16f:
            return Backend::IL::Format::RGBA16Float;
        case SpvImageFormatR32f:
            return Backend::IL::Format::R32Float;
        case SpvImageFormatRgba8:
            return Backend::IL::Format::RGBA8;
        case SpvImageFormatRgba8Snorm:
            return Backend::IL::Format::RGBA8Snorm;
        case SpvImageFormatRg32f:
            return Backend::IL::Format::RG32Float;
        case SpvImageFormatRg16f:
            return Backend::IL::Format::RG16Float;
        case SpvImageFormatR11fG11fB10f:
            return Backend::IL::Format::R11G11B10Float;
        case SpvImageFormatR16f:
            return Backend::IL::Format::R16Float;
        case SpvImageFormatRgba16:
            return Backend::IL::Format::RGBA16;
        case SpvImageFormatRgb10A2:
            return Backend::IL::Format::RGB10A2;
        case SpvImageFormatRg16:
            return Backend::IL::Format::RG16;
        case SpvImageFormatRg8:
            return Backend::IL::Format::RG8;
        case SpvImageFormatR16:
            return Backend::IL::Format::R16;
        case SpvImageFormatR8:
            return Backend::IL::Format::R8;
        case SpvImageFormatRgba16Snorm:
            return Backend::IL::Format::RGBA16Snorm;
        case SpvImageFormatRg16Snorm:
            return Backend::IL::Format::RG16Snorm;
        case SpvImageFormatRg8Snorm:
            return Backend::IL::Format::RG8Snorm;
        case SpvImageFormatR16Snorm:
            return Backend::IL::Format::R16Snorm;
        case SpvImageFormatR8Snorm:
            return Backend::IL::Format::R8Snorm;
        case SpvImageFormatRgba32i:
            return Backend::IL::Format::RGBA32Int;
        case SpvImageFormatRgba16i:
            return Backend::IL::Format::RGBA16Int;
        case SpvImageFormatRgba8i:
            return Backend::IL::Format::RGBA8Int;
        case SpvImageFormatR32i:
            return Backend::IL::Format::R32Int;
        case SpvImageFormatRg32i:
            return Backend::IL::Format::RG32Int;
        case SpvImageFormatRg16i:
            return Backend::IL::Format::RG16Int;
        case SpvImageFormatRg8i:
            return Backend::IL::Format::RG8Int;
        case SpvImageFormatR16i:
            return Backend::IL::Format::R16Int;
        case SpvImageFormatR8i:
            return Backend::IL::Format::R8Int;
        case SpvImageFormatRgba32ui:
            return Backend::IL::Format::RGBA32UInt;
        case SpvImageFormatRgba16ui:
            return Backend::IL::Format::RGBA16UInt;
        case SpvImageFormatRgba8ui:
            return Backend::IL::Format::RGBA8UInt;
        case SpvImageFormatR32ui:
            return Backend::IL::Format::R32UInt;
        case SpvImageFormatRgb10a2ui:
            return Backend::IL::Format::RGB10a2UInt;
        case SpvImageFormatRg32ui:
            return Backend::IL::Format::RG32UInt;
        case SpvImageFormatRg16ui:
            return Backend::IL::Format::RG16UInt;
        case SpvImageFormatRg8ui:
            return Backend::IL::Format::RG8UInt;
        case SpvImageFormatR16ui:
            return Backend::IL::Format::R16UInt;
        case SpvImageFormatR8ui:
            return Backend::IL::Format::R8UInt;
    }
}

inline SpvStorageClass Translate(Backend::IL::AddressSpace space) {
    switch (space) {
        default:
            ASSERT(false, "Invalid address space");
            return SpvStorageClassGeneric;
        case Backend::IL::AddressSpace::Texture:
        case Backend::IL::AddressSpace::Buffer:
            return SpvStorageClassImage;
        case Backend::IL::AddressSpace::Function:
            return SpvStorageClassFunction;
        case Backend::IL::AddressSpace::Resource:
            return SpvStorageClassUniformConstant;
        case Backend::IL::AddressSpace::Constant:
            return SpvStorageClassUniform;
        case Backend::IL::AddressSpace::RootConstant:
            return SpvStorageClassPushConstant;
    }
}

inline Backend::IL::AddressSpace Translate(SpvStorageClass space) {
    switch (space) {
        default:
            return Backend::IL::AddressSpace::Unexposed;
        case SpvStorageClassFunction:
            return Backend::IL::AddressSpace::Function;
        case SpvStorageClassImage:
            return Backend::IL::AddressSpace::Texture;
        case SpvStorageClassUniformConstant:
            return Backend::IL::AddressSpace::Resource;
        case SpvStorageClassPushConstant:
            return Backend::IL::AddressSpace::RootConstant;
        case SpvStorageClassUniform:
            return Backend::IL::AddressSpace::Constant;
        case SpvStorageClassOutput:
            return Backend::IL::AddressSpace::Output;
    }
}

inline Backend::IL::TextureDimension Translate(SpvDim dim) {
    switch (dim) {
        default:
            return Backend::IL::TextureDimension::Unexposed;
        case SpvDim1D:
            return Backend::IL::TextureDimension::Texture1D;
        case SpvDim2D:
            return Backend::IL::TextureDimension::Texture2D;
        case SpvDim3D:
            return Backend::IL::TextureDimension::Texture3D;
        case SpvDimCube:
            return Backend::IL::TextureDimension::Texture2DCube;
        case SpvDimSubpassData:
            return Backend::IL::TextureDimension::SubPass;
    }
}