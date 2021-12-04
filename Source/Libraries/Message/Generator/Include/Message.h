#pragma once

#include "AttributeMap.h"
#include "Field.h"

#include <vector>

/// Single message type
struct Message {
    /// Specialization type of this message
    std::string type;

    /// Name of this message
    std::string name;

    /// All attributes of this message
    AttributeMap attributes;

    /// All fields of this message
    std::vector<Field> fields;

    /// Source line
    uint32_t line;
};
