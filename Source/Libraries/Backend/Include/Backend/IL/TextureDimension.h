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
        Unexposed,
    };

    /// Get the dimension size / count of the mdoe
    inline uint32_t GetDimensionSize(TextureDimension dim) {
        switch (dim) {
            default:
                ASSERT(false, "Invalid dimension");
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
                return 2;
            case TextureDimension::Texture2DCubeArray:
                return 3;
        }
    }
}
