#pragma once

// Backend
#include "ResourceToken.h"

struct BufferDescriptor {
    /// Offset within the resource
    uint64_t offset;

    /// Width of the resource
    uint64_t width;

    /// Unique identifier of the resource
    uint64_t uid;
};
