#pragma once

// Layer
#include "DescriptorLayoutPhysicalMapping.h"

// Std
#include <cstdint>

struct PipelineLayoutPhysicalMapping {
    /// Mapping hash
    uint64_t layoutHash{0};

    /// All laid out descriptor sets
    std::vector<DescriptorLayoutPhysicalMapping> descriptorSets;
};
