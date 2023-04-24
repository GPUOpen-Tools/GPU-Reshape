#pragma once

// Std
#include <cstdint>

class ShaderBlob {
public:
    /// Byte code of this shader
    const void *code{nullptr};

    /// Byte length of this shader
    uint64_t length{0};
};
