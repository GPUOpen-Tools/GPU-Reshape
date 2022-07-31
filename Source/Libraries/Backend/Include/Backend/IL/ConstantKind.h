#pragma once

// Std
#include <cstdint>

namespace Backend::IL {
    enum class ConstantKind : uint8_t {
        None,
        Undef,
        Bool,
        Int,
        FP,
        Unexposed
    };
}
