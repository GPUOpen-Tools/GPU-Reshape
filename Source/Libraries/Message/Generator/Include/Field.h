#pragma once

// Generator
#include "AttributeMap.h"

// Std
#include <vector>

/// Message field
struct Field {
    /// Name of this field
    std::string name;

    /// Type of this field, may reference generated types
    std::string type;

    /// All attributes of this field
    AttributeMap attributes;

    /// Source line
    uint32_t line{};
};
