#pragma once

// Std
#include <cstdint>

struct PipelineLayoutBindingInfo {
    /// Counter descriptor
    uint32_t counterDescriptorOffset{0};

    /// Stream descriptors
    uint32_t streamDescriptorOffset{0};
    uint32_t streamDescriptorCount{0};

    /// PRMT descriptors
    uint32_t prmtDescriptorOffset{0};

    /// Shader data descriptors
    uint32_t shaderDataDescriptorOffset{0};
    uint32_t shaderDataDescriptorCount{0};

    /// Export push constants
    uint32_t exportPushConstantOffset{0};
};