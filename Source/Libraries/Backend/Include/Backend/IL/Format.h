#pragma once

namespace Backend::IL {
    enum class Format {
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
    };

    /// Get the byte size of a format
    inline uint8_t GetSize(Format format) {
        switch (format) {
            default:
                return 0;
            case Format::RGBA32Float:
                return 16;
            case Format::RGBA16Float:
                return 8;
            case Format::R32Float:
                return 4;
            case Format::RGBA8:
                return 4;
            case Format::RGBA8Snorm:
                return 4;
            case Format::RG32Float:
                return 4;
            case Format::RG16Float:
                return 2;
            case Format::R11G11B10Float:
                return 4;
            case Format::R16Float:
                return 2;
            case Format::RGBA16:
                return 8;
            case Format::RGB10A2:
                return 4;
            case Format::RG16:
                return 4;
            case Format::RG8:
                return 2;
            case Format::R16:
                return 2;
            case Format::R8:
                return 1;
            case Format::RGBA16Snorm:
                return 8;
            case Format::RG16Snorm:
                return 4;
            case Format::RG8Snorm:
                return 2;
            case Format::R16Snorm:
                return 2;
            case Format::R8Snorm:
                return 1;
            case Format::RGBA32Int:
                return 16;
            case Format::RGBA16Int:
                return 8;
            case Format::RGBA8Int:
                return 4;
            case Format::R32Int:
                return 4;
            case Format::RG32Int:
                return 4;
            case Format::RG16Int:
                return 4;
            case Format::RG8Int:
                return 2;
            case Format::R16Int:
                return 2;
            case Format::R8Int:
                return 1;
            case Format::RGBA32UInt:
                return 16;
            case Format::RGBA16UInt:
                return 8;
            case Format::RGBA8UInt:
                return 4;
            case Format::R32UInt:
                return 4;
            case Format::RGB10a2UInt:
                return 4;
            case Format::RG32UInt:
                return 8;
            case Format::RG16UInt:
                return 4;
            case Format::RG8UInt:
                return 2;
            case Format::R16UInt:
                return 4;
            case Format::R8UInt:
                return 1;
        }
    }
}
