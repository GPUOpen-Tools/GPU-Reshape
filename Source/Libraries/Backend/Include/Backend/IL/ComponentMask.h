#pragma once

// Common
#include <Common/Enum.h>

// Std
#include <cstdint>

namespace IL {
    enum class ComponentMask : uint8_t {
        X = BIT(0),
        Y = BIT(1),
        Z = BIT(2),
        W = BIT(3),
        Red = X,
        Green = Y,
        Blue = Z,
        Alpha = W,
        All = X | Y | Z | W
    };

    BIT_SET(ComponentMask);
}
