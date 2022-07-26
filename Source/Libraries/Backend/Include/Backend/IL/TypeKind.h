#pragma once

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
        Compound,
        Pointer,
        Array,
        Texture,
        Buffer,
        Function,
        Struct,
        Unexposed
    };
}
