#pragma once

// Layer
#include <Backends/Vulkan/Vulkan.h>

// Std
#include <cstdint>

struct ShaderModuleInstrumentationKey {
    auto AsTuple() const {
#if PRMT_METHOD == PRMT_METHOD_UB_PC
        return std::make_tuple(pipelineLayoutUserSlots, featureBitSet, pipelineLayoutDataPCOffset, pipelineLayoutPRMTPCOffset);
#else // PRMT_METHOD == PRMT_METHOD_UB_PC
        return std::make_tuple(pipelineLayoutUserSlots, featureBitSet, pipelineLayoutDataPCOffset);
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC
    }

    bool operator<(const ShaderModuleInstrumentationKey& key) const {
        return AsTuple() < key.AsTuple();
    }

    /// Number of pipeline layout user bound descriptor sets
    uint32_t pipelineLayoutUserSlots{0};

    /// Data push constant offset after the user PC data
    uint32_t pipelineLayoutDataPCOffset{0};

#if PRMT_METHOD == PRMT_METHOD_UB_PC
    /// PRMT push constant offset after the user PC data
    uint32_t pipelineLayoutPRMTPCOffset{0};
#endif // PRMT_METHOD == PRMT_METHOD_UB_PC

    /// Feature bit set
    uint64_t featureBitSet{0};
};
