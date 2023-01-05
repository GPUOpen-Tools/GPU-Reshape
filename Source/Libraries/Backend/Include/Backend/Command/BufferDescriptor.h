#pragma once

// Backend
#include "ResourceToken.h"

struct BufferDescriptor {
    /// Shader token
    ResourceToken token;

    /// Offset within the resource
    uint64_t offset;

    /// Unique identifier of the resource
    uint64_t uid;
};
