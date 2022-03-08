#pragma once

// Std
#include <cstdint>

namespace Backend::IL {
    enum class ResourceSamplerMode : uint8_t {
        RuntimeOnly,
        Compatible,
        Writable
    };
}