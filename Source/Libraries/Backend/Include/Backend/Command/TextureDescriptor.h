#pragma once

// Backend
#include "ResourceToken.h"
#include "TextureRegion.h"

struct TextureDescriptor {
    /// Region information
    TextureRegion region;

    /// Unique identifier of the resource
    uint64_t uid;
};
