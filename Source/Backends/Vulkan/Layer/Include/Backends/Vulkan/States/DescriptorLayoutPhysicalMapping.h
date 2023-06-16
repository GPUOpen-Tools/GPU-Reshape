#pragma once

// Layer
#include "BindingPhysicalMapping.h"

// Std
#include <vector>

struct DescriptorLayoutPhysicalMapping {
    /// All mappings
    std::vector<BindingPhysicalMapping> bindings;
};
