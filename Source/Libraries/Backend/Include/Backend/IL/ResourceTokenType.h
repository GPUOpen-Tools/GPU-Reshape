#pragma once

#include <cstdint>

namespace Backend::IL {
    enum class ResourceTokenType {
        Texture,
        Buffer,
        CBuffer,
        Sampler
    };
}
