#pragma once

#include <cstdint>

namespace IL {
    struct RelocationOffset {
        /// Offset from the start of the instruction stream
        uint32_t offset;
    };
}
