#pragma once

// Std
#include <cstdint>
#include <tuple>

struct ShaderInstrumentationKey {
    auto AsTuple() const {
        return std::make_tuple(featureBitSet);
    }

    bool operator<(const ShaderInstrumentationKey& key) const {
        return AsTuple() < key.AsTuple();
    }

    /// Feature bit set
    uint64_t featureBitSet{0};
};
