#pragma once

#include <cstdint>

namespace IL {
    struct CapabilityTable {
        /// Does the program use structured control flow?
        bool hasControlFlow = false;

        /// Does the program represent integer signs as unique types?
        bool integerSignIsUnique = true;
    };
}
