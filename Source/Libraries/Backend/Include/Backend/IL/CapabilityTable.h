#pragma once

#include <cstdint>

namespace IL {
    struct CapabilityTable {
        /// Does the program use structured control flow?
        bool hasControlFlow = false;
    };
}