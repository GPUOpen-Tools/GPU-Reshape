#pragma once

// Backend
#include "ShaderResourceType.h"
#include "ShaderBufferInfo.h"
#include "ShaderResource.h"

struct ShaderResourceInfo {
    ShaderResourceInfo() : buffer({}) {
        /* poof */
    }

    /// Identifier of the resource
    ShaderResourceID id;

    /// Type of the resource
    ShaderResourceType type;

    /// Payload
    union
    {
        ShaderBufferInfo buffer;
    };
};
