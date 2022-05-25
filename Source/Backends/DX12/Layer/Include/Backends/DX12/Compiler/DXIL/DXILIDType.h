#pragma once

// Std
#include <cstdint>

enum class DXILIDType : uint8_t {
    Instruction = 0,
    Variable,
    Alias,
    Function,
    Constant,
    Parameter
};
