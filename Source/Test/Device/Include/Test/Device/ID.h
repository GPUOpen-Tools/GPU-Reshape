#pragma once

#include <Common/UniqueID.h>

namespace Test {
    UNIQUE_ID(uint32_t, ResourceID);
    UNIQUE_ID(uint32_t, ResourceLayoutID);
    UNIQUE_ID(uint32_t, ResourceSetID);
    UNIQUE_ID(uint32_t, QueueID);
    UNIQUE_ID(uint32_t, CommandBufferID);
    UNIQUE_ID(uint32_t, PipelineID);

    UNIQUE_ID(ResourceID, BufferID);
    UNIQUE_ID(ResourceID, TextureID);
    UNIQUE_ID(ResourceID, SamplerID);
    UNIQUE_ID(ResourceID, CBufferID);
}
