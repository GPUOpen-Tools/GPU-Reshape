#pragma once

// Common
#include <Common/Enum.h>

namespace IL {
    /// Visitation flag
    enum class VisitFlag {
        /// Continue traversal, default
        Continue = 0x0,

        /// Stop traversal
        Stop = BIT(1),
    };

    /// Set of flags
    BIT_SET(VisitFlag);
}