#pragma once

namespace Backend::IL {
    enum class TextureDimension {
        Texture1D,
        Texture2D,
        Texture3D,
        Texture1DArray,
        Texture2DArray,
        Texture2DCube,
        TexelBuffer,
        Unexposed,
    };
}
