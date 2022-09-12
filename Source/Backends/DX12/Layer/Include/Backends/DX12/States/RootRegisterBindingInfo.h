#pragma once

// Std
#include <cstdint>

struct RootRegisterBindingInfo {
    /// Space index
    uint32_t space{~0u};

    /// Base register index
    uint32_t _register{~0u};
};
