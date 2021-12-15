#pragma once

// Common
#include <Common/Enum.h>

enum class ShaderExportAllocationMode : uint8_t {
    /// Allocate stream data for each command buffer
    LocalCommandBuffer,

    /// Allocate stream data for all command buffers, cyclic buffer
    GlobalCyclicBufferNoOverwrite,
};
