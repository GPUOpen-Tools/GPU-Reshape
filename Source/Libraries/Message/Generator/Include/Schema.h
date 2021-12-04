#pragma once

// Generator
#include "Message.h"

// Std
#include <list>

/// Generator schema
struct Schema {
    /// All messages within this schema
    std::list<Message> messages;
};
