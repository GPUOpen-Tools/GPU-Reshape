#pragma once

#include <cstdint>

struct ShaderModuleInstrumentationKey {
    auto AsTuple() const {
        return std::make_tuple(pipelineLayoutUserSlots, featureBitSet, pipelineLayoutUserPCOffset);
    }

    bool operator<(const ShaderModuleInstrumentationKey& key) const {
        return AsTuple() < key.AsTuple();
    }

    /// Number of pipeline layout user bound descriptor sets
    uint32_t pipelineLayoutUserSlots{0};

    /// Push constant offset after the user PC data
    uint32_t pipelineLayoutUserPCOffset{0};

    /// Feature bit set
    uint64_t featureBitSet{0};
};
