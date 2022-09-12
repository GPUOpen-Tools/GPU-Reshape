#pragma once

// Common
#include <Common/Enum.h>

enum class ShaderExportAllocationMode : uint8_t {
    /// Allocate stream data for each command list
    LocalCommandList,

    /// Allocate stream data for all command lists, cyclic buffer
    GlobalCyclicBufferNoOverwrite,
};
