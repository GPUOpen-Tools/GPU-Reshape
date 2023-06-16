#pragma once

// Std
#include <cstdint>

struct BindingPhysicalMapping {
    BindingPhysicalMapping() : immutableSamplers(0), prmtOffset(0) {
        
    }
    
    /// Underlying type
    VkDescriptorType type{};

    /// Number of bindings
    uint32_t bindingCount{0};

    /// Are the samplers immutable?
    uint32_t immutableSamplers : 1;
    
    /// Precomputed PRMT offset
    uint32_t prmtOffset : 31;
};
