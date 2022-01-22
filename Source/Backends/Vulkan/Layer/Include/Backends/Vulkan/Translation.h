#pragma once

// Layer
#include "Vulkan.h"

// Common
#include <Common/Assert.h>

// Backend
#include <Backend/IL/Format.h>
#include <Backend/IL/AddressSpace.h>
#include <Backend/IL/TextureDimension.h>

inline VkFormat Translate(Backend::IL::Format format) {
    switch (format) {
        default:
        ASSERT(false, "Invalid format");
            return VK_FORMAT_UNDEFINED;
        case Backend::IL::Format::RGBA32Float:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case Backend::IL::Format::RGBA16Float:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Backend::IL::Format::R32Float:
            return VK_FORMAT_R32_SFLOAT;
        case Backend::IL::Format::RGBA8:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case Backend::IL::Format::RGBA8Snorm:
            return VK_FORMAT_R8G8B8A8_SNORM;
        case Backend::IL::Format::RG32Float:
            return VK_FORMAT_R32G32_SFLOAT;
        case Backend::IL::Format::RG16Float:
            return VK_FORMAT_R16G16_SFLOAT;
        case Backend::IL::Format::R11G11B10Float:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case Backend::IL::Format::R16Float:
            return VK_FORMAT_R16_SFLOAT;
        case Backend::IL::Format::RGBA16:
            return VK_FORMAT_R16G16B16A16_UNORM;
        case Backend::IL::Format::RGB10A2:
            return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
        case Backend::IL::Format::RG16:
            return VK_FORMAT_R16G16_UNORM;
        case Backend::IL::Format::RG8:
            return VK_FORMAT_R8G8_UNORM;
        case Backend::IL::Format::R16:
            return VK_FORMAT_R16_UNORM;
        case Backend::IL::Format::R8:
            return VK_FORMAT_R8_UNORM;
        case Backend::IL::Format::RGBA16Snorm:
            return VK_FORMAT_R16G16B16A16_SNORM;
        case Backend::IL::Format::RG16Snorm:
            return VK_FORMAT_R16G16_SNORM;
        case Backend::IL::Format::RG8Snorm:
            return VK_FORMAT_R8G8_SNORM;
        case Backend::IL::Format::R16Snorm:
            return VK_FORMAT_R16_SNORM;
        case Backend::IL::Format::R8Snorm:
            return VK_FORMAT_R8_SNORM;
        case Backend::IL::Format::RGBA32Int:
            return VK_FORMAT_R32G32B32_SINT;
        case Backend::IL::Format::RGBA16Int:
            return VK_FORMAT_R16G16B16A16_SINT;
        case Backend::IL::Format::RGBA8Int:
            return VK_FORMAT_R8G8B8A8_SINT;
        case Backend::IL::Format::R32Int:
            return VK_FORMAT_R32_SINT;
        case Backend::IL::Format::RG32Int:
            return VK_FORMAT_R32G32_SINT;
        case Backend::IL::Format::RG16Int:
            return VK_FORMAT_R16G16_SINT;
        case Backend::IL::Format::RG8Int:
            return VK_FORMAT_R8G8_SINT;
        case Backend::IL::Format::R16Int:
            return VK_FORMAT_R16_SINT;
        case Backend::IL::Format::R8Int:
            return VK_FORMAT_R8_SINT;
        case Backend::IL::Format::RGBA32UInt:
            return VK_FORMAT_R32G32B32A32_UINT;
        case Backend::IL::Format::RGBA16UInt:
            return VK_FORMAT_R16G16B16A16_UINT;
        case Backend::IL::Format::RGBA8UInt:
            return VK_FORMAT_R8G8B8A8_UINT;
        case Backend::IL::Format::R32UInt:
            return VK_FORMAT_R32_UINT;
        case Backend::IL::Format::RGB10a2UInt:
            return VK_FORMAT_A2R10G10B10_UINT_PACK32;
        case Backend::IL::Format::RG32UInt:
            return VK_FORMAT_R32G32_UINT;
        case Backend::IL::Format::RG16UInt:
            return VK_FORMAT_R16G16_UINT;
        case Backend::IL::Format::RG8UInt:
            return VK_FORMAT_R8G8_UINT;
        case Backend::IL::Format::R16UInt:
            return VK_FORMAT_R16_UINT;
        case Backend::IL::Format::R8UInt:
            return VK_FORMAT_R8_UINT;
    }
}

inline Backend::IL::Format Translate(VkFormat format) {
    switch (format) {
        default:
            return Backend::IL::Format::None;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return Backend::IL::Format::RGBA32Float;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return Backend::IL::Format::RGBA16Float;
        case VK_FORMAT_R32_SFLOAT:
            return Backend::IL::Format::R32Float;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return Backend::IL::Format::RGBA8;
        case VK_FORMAT_R8G8B8A8_SNORM:
            return Backend::IL::Format::RGBA8Snorm;
        case VK_FORMAT_R32G32_SFLOAT:
            return Backend::IL::Format::RG32Float;
        case VK_FORMAT_R16G16_SFLOAT:
            return Backend::IL::Format::RG16Float;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return Backend::IL::Format::R11G11B10Float;
        case VK_FORMAT_R16_SFLOAT:
            return Backend::IL::Format::R16Float;
        case VK_FORMAT_R16G16B16A16_UNORM:
            return Backend::IL::Format::RGBA16;
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
            return Backend::IL::Format::RGB10A2;
        case VK_FORMAT_R16G16_UNORM:
            return Backend::IL::Format::RG16;
        case VK_FORMAT_R8G8_UNORM:
            return Backend::IL::Format::RG8;
        case VK_FORMAT_R16_UNORM:
            return Backend::IL::Format::R16;
        case VK_FORMAT_R8_UNORM:
            return Backend::IL::Format::R8;
        case VK_FORMAT_R16G16B16A16_SNORM:
            return Backend::IL::Format::RGBA16Snorm;
        case VK_FORMAT_R16G16_SNORM:
            return Backend::IL::Format::RG16Snorm;
        case VK_FORMAT_R8G8_SNORM:
            return Backend::IL::Format::RG8Snorm;
        case VK_FORMAT_R16_SNORM:
            return Backend::IL::Format::R16Snorm;
        case VK_FORMAT_R8_SNORM:
            return Backend::IL::Format::R8Snorm;
        case VK_FORMAT_R32G32B32_SINT:
            return Backend::IL::Format::RGBA32Int;
        case VK_FORMAT_R16G16B16A16_SINT:
            return Backend::IL::Format::RGBA16Int;
        case VK_FORMAT_R8G8B8A8_SINT:
            return Backend::IL::Format::RGBA8Int;
        case VK_FORMAT_R32_SINT:
            return Backend::IL::Format::R32Int;
        case VK_FORMAT_R32G32_SINT:
            return Backend::IL::Format::RG32Int;
        case VK_FORMAT_R16G16_SINT:
            return Backend::IL::Format::RG16Int;
        case VK_FORMAT_R8G8_SINT:
            return Backend::IL::Format::RG8Int;
        case VK_FORMAT_R16_SINT:
            return Backend::IL::Format::R16Int;
        case VK_FORMAT_R8_SINT:
            return Backend::IL::Format::R8Int;
        case VK_FORMAT_R32G32B32A32_UINT:
            return Backend::IL::Format::RGBA32UInt;
        case VK_FORMAT_R16G16B16A16_UINT:
            return Backend::IL::Format::RGBA16UInt;
        case VK_FORMAT_R8G8B8A8_UINT:
            return Backend::IL::Format::RGBA8UInt;
        case VK_FORMAT_R32_UINT:
            return Backend::IL::Format::R32UInt;
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            return Backend::IL::Format::RGB10a2UInt;
        case VK_FORMAT_R32G32_UINT:
            return Backend::IL::Format::RG32UInt;
        case VK_FORMAT_R16G16_UINT:
            return Backend::IL::Format::RG16UInt;
        case VK_FORMAT_R8G8_UINT:
            return Backend::IL::Format::RG8UInt;
        case VK_FORMAT_R16_UINT:
            return Backend::IL::Format::R16UInt;
        case VK_FORMAT_R8_UINT:
            return Backend::IL::Format::R8UInt;
    }
}

inline VkImageType Translate(Backend::IL::TextureDimension dim) {
    switch (dim) {
        default:
        ASSERT(false, "Invalid dimension");
            return VK_IMAGE_TYPE_MAX_ENUM;
        case Backend::IL::TextureDimension::Texture1D:
        case Backend::IL::TextureDimension::Texture1DArray:
            return VK_IMAGE_TYPE_1D;
        case Backend::IL::TextureDimension::Texture2D:
        case Backend::IL::TextureDimension::Texture2DArray:
        case Backend::IL::TextureDimension::Texture2DCube:
            return VK_IMAGE_TYPE_2D;
        case Backend::IL::TextureDimension::Texture3D:
            return VK_IMAGE_TYPE_3D;
    }
}

inline VkImageViewType TranslateViewType(Backend::IL::TextureDimension dim) {
    switch (dim) {
        default:
        ASSERT(false, "Invalid dimension");
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        case Backend::IL::TextureDimension::Texture1D:
            return VK_IMAGE_VIEW_TYPE_1D;
        case Backend::IL::TextureDimension::Texture1DArray:
            return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case Backend::IL::TextureDimension::Texture2D:
            return VK_IMAGE_VIEW_TYPE_2D;
        case Backend::IL::TextureDimension::Texture2DArray:
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case Backend::IL::TextureDimension::Texture3D:
            return VK_IMAGE_VIEW_TYPE_3D;
        case Backend::IL::TextureDimension::Texture2DCube:
            return VK_IMAGE_VIEW_TYPE_CUBE;
    }
}
