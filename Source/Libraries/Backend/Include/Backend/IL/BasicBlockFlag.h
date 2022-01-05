#pragma once

// Common
#include <Common/Enum.h>

enum class BasicBlockFlag {
    None = 0x0,

    /// Do not perform instrumentation on this block
    NoInstrumentation = BIT(1),

    /// Block has been visited
    Visited = BIT(2),
};

BIT_SET(BasicBlockFlag);
