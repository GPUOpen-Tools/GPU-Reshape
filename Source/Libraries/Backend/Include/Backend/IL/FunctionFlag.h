#pragma once

// Common
#include <Common/Enum.h>

enum class FunctionFlag {
    None = 0x0,

    /// Do not perform instrumentation on this function
    NoInstrumentation = BIT(1),

    /// Function has been visited
    Visited = BIT(2),
};

BIT_SET(FunctionFlag);
