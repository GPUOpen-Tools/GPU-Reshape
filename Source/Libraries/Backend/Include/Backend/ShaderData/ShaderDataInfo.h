#pragma once

// Backend
#include "ShaderDataType.h"
#include "ShaderDataBufferInfo.h"
#include "ShaderDataEventInfo.h"
#include "ShaderData.h"

struct ShaderDataInfo {
    ShaderDataInfo() : buffer({}) {
        /* poof */
    }

    /// Identifier of the data
    ShaderDataID id;

    /// Type of the data
    ShaderDataType type;

    /// Payload
    union
    {
        ShaderDataBufferInfo buffer;
        ShaderDataEventInfo event;
    };
};
