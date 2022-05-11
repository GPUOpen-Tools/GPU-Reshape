#pragma once

// Std
#include <cstdint>
#include <tuple>

struct ShaderInstrumentationKey {
    auto AsTuple() const {
        return std::make_tuple(pipelineLayoutUserSlots, featureBitSet);
    }

    bool operator<(const ShaderInstrumentationKey& key) const {
        return AsTuple() < key.AsTuple();
    }

    /// Number of pipeline layout user bound descriptor sets
    uint32_t pipelineLayoutUserSlots{0};

    /// Feature bit set
    uint64_t featureBitSet{0};
};
