#pragma once

namespace Test {
    enum class ResourceType {
        TexelBuffer,
        RWTexelBuffer,
        Texture1D,
        RWTexture1D,
        Texture2D,
        RWTexture2D,
        Texture3D,
        RWTexture3D,
        SamplerState,
        CBuffer
    };
}
