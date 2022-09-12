#pragma once

// Layer
#include "RootRegisterBindingInfo.h"

// Std
#include <cstdint>
#include <tuple>

struct ShaderInstrumentationKey {
    auto AsTuple() const {
        return std::make_tuple(featureBitSet, bindingInfo.space, bindingInfo._register);
    }

    bool operator<(const ShaderInstrumentationKey& key) const {
        return AsTuple() < key.AsTuple();
    }

    /// Feature bit set
    uint64_t featureBitSet{0};

    /// Signature root binding info
    RootRegisterBindingInfo bindingInfo{};
};
