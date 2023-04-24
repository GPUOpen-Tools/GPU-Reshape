#pragma once

// Std
#include <cstdint>

namespace IL {
    /// Token physical unique id, limit of 4'194'304 resources (22 bits)
    static constexpr uint32_t kResourceTokenPUIDShift = 0u;
    static constexpr uint32_t kResourceTokenPUIDBitCount = 22u;
    static constexpr uint32_t kResourceTokenPUIDMask  = (1u << kResourceTokenPUIDBitCount) - 1u;

    /// Special physical unique ids
    static constexpr uint32_t kResourceTokenPUIDUndefined    = kResourceTokenPUIDMask - 0;
    static constexpr uint32_t kResourceTokenPUIDOutOfBounds  = kResourceTokenPUIDMask - 1;
    static constexpr uint32_t kResourceTokenPUIDInvalidStart = kResourceTokenPUIDOutOfBounds;

    /// Token type, texture, buffer, cbuffer or sampler (2 bits)
    static constexpr uint32_t kResourceTokenTypeShift = 22u;
    static constexpr uint32_t kResourceTokenTypeBitCount = 2u;
    static constexpr uint32_t kResourceTokenTypeMask  = (1u << kResourceTokenTypeBitCount) - 1u;

    /// Token sub-resource base, base subresource offset into the designated resource (8 bits)
    static constexpr uint32_t kResourceTokenSRBShift = 24u;
    static constexpr uint32_t kResourceTokenSRBBitCount = 8u;
    static constexpr uint32_t kResourceTokenSRBMask  = (1u << kResourceTokenSRBBitCount) - 1u;
}
