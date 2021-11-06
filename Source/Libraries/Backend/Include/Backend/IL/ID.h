#pragma once

#include <cstdint>

namespace IL {
    using Opaque = uint64_t;

    using ID = uint32_t;

    static constexpr ID InvalidID = ~0u;
}
