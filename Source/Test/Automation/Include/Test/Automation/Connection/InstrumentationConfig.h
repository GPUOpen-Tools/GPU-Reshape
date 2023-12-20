#pragma once

// Std
#include <cstdint>

struct InstrumentationConfig {
    /// Feature set to enable
    uint64_t featureBitSet{0ull};
};
