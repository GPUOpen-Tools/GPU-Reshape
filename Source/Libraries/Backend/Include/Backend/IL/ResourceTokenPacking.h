#pragma once

// Std
#include <cstdint>

namespace IL {
    /// Token physical unique id, limit of 4'194'304 resources (22 bits)
    static constexpr uint32_t kResourceTokenPUIDShift = 0u;
    static constexpr uint32_t kResourceTokenPUIDBitCount = 22u;
    static constexpr uint32_t kResourceTokenPUIDMask  = (1u << kResourceTokenPUIDBitCount) - 1u;

    /// Unmapped and invalid physical unique ids
    static constexpr uint32_t kResourceTokenPUIDInvalidUndefined    = kResourceTokenPUIDMask - 0;
    static constexpr uint32_t kResourceTokenPUIDInvalidOutOfBounds  = kResourceTokenPUIDMask - 1;
    static constexpr uint32_t kResourceTokenPUIDInvalidStart        = kResourceTokenPUIDInvalidOutOfBounds;

    /// Reserved physical unique ids
    static constexpr uint32_t kResourceTokenPUIDReservedNullTexture = 0;
    static constexpr uint32_t kResourceTokenPUIDReservedNullBuffer  = 1;
    static constexpr uint32_t kResourceTokenPUIDReservedNullCBuffer = 2;
    static constexpr uint32_t kResourceTokenPUIDReservedNullSampler = 3;
    static constexpr uint32_t kResourceTokenPUIDReservedCount       = 4;

    /// Token type, texture, buffer, cbuffer or sampler (2 bits)
    static constexpr uint32_t kResourceTokenTypeShift = 22u;
    static constexpr uint32_t kResourceTokenTypeBitCount = 2u;
    static constexpr uint32_t kResourceTokenTypeMask  = (1u << kResourceTokenTypeBitCount) - 1u;

    /// Token sub-resource base, base subresource offset into the designated resource (8 bits)
    static constexpr uint32_t kResourceTokenSRBShift = 24u;
    static constexpr uint32_t kResourceTokenSRBBitCount = 8u;
    static constexpr uint32_t kResourceTokenSRBMask  = (1u << kResourceTokenSRBBitCount) - 1u;
}
