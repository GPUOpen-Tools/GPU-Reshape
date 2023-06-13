#pragma once

// Std
#include <vector>
#include <cstdint>

struct DescriptorLayoutPhysicalMapping {
    /// Precomputed PRMT offsets
    std::vector<uint32_t> prmtOffsets;
};
