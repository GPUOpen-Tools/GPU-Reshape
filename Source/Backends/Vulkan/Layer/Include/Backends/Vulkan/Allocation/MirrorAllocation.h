#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>
#include "Allocation.h"

/// An allocation that is potentially mirrored on the CPU
struct MirrorAllocation {
    /// Device object, non cpu-readable
    Allocation device;

    /// Host object, cpu-readable, mapping behaviour up to allocation
    Allocation host;
};
