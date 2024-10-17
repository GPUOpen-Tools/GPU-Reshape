#pragma once

// Std
#include <cstdint>

namespace IL {
    enum class WorkGroupDivergence : uint8_t {
        /// No divergence status known
        Unknown,

        /// The value is uniform within the work group
        Uniform,

        /// The value is divergent within the work group
        Divergent
    };

    inline WorkGroupDivergence AsDivergence(bool divergent) {
        return divergent ? WorkGroupDivergence::Divergent : WorkGroupDivergence::Uniform;
    }
}
