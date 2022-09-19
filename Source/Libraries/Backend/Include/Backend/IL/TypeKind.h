#pragma once

// Std
#include <cstdint>

namespace Backend::IL {
    enum class TypeKind : uint8_t {
        None,
        Bool,
        Void,
        Int,
        FP,
        Vector,
        Matrix,
        Pointer,
        Array,
        Texture,
        Buffer,
        Sampler,
        CBuffer,
        Function,
        Struct,
        Unexposed
    };
}
