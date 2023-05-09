#pragma once

// Layer
#include "RootRegisterBindingInfo.h"
#include "RootSignaturePhysicalMapping.h"

// Std
#include <cstdint>
#include <tuple>

struct ShaderInstrumentationKey {
    auto AsTuple() const {
        return std::make_tuple(featureBitSet, combinedHash);
    }

    bool operator<(const ShaderInstrumentationKey& key) const {
        return AsTuple() < key.AsTuple();
    }

    /// Feature bit set
    uint64_t featureBitSet{0};

    /// Final hash, includes stream data and physical mappings
    uint64_t combinedHash{0};

    /// Root signature mapping
    RootSignaturePhysicalMapping* physicalMapping{nullptr};

    /// Signature root binding info
    RootRegisterBindingInfo bindingInfo{};
};
