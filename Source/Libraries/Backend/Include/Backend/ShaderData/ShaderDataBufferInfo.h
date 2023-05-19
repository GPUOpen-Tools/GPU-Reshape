#pragma once

// Backend
#include <Backend/IL/Format.h>

// Std
#include <cstdint>

struct ShaderDataBufferInfo {
    /// Number of elements within this buffer
    size_t elementCount{0};

    /// Format of each element
    Backend::IL::Format format{Backend::IL::Format::None};

    /// Is this buffer visible from the host?
    bool hostVisible{false};
};
