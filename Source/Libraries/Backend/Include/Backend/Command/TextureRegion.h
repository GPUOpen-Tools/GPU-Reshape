#pragma once

// Std
#include <cstdint>

struct TextureRegion {
    /// Subresource
    uint32_t baseMip{0};
    uint32_t baseSlice{0};

    /// Regional offsets
    uint32_t offsetX{0};
    uint32_t offsetY{0};
    uint32_t offsetZ{0};

    /// Regional sizes
    uint32_t width{0};
    uint32_t height{0};
    uint32_t depth{0};
};
