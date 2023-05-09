#pragma once

// Message
#include "AttributeMap.h"
#include "Field.h"

// Std
#include <vector>

/// Single message type
struct Chunk {
    /// Name of this chunk
    std::string name;

    /// All attributes of this chunk
    AttributeMap attributes;

    /// All fields of this chunk
    std::vector<Field> fields;

    /// Source line
    uint32_t line;
};
