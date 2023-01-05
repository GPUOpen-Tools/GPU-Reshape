#pragma once

// Backend
#include <Backend/IL/ResourceTokenType.h>

// Std
#include <cstdint>

/// Unpacked token type
struct ResourceToken {
    /// Physical UID of the resource
    uint64_t puid;

    /// Type of the resource
    Backend::IL::ResourceTokenType type;

    /// Sub-resource base
    uint32_t srb;
};
