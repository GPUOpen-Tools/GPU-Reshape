#pragma once

// Backend
#include <Backend/IL/ResourceTokenPacking.h>

// Std
#include <cstdint>

struct VirtualResourceMapping {
    union
    {
        struct
        {
            /// Physical UID of the resource
            uint32_t puid : IL::kResourceTokenPUIDBitCount;

            /// Type identifier of this resource
            uint32_t type : IL::kResourceTokenTypeBitCount;

            /// Sub-resource base of this resource
            uint32_t srb  : IL::kResourceTokenSRBBitCount;
        };

        /// Opaque key
        uint32_t opaque;
    };
};

/// Validation
static_assert(sizeof(VirtualResourceMapping) == sizeof(uint32_t), "Unexpected virtual resource mapping size");
