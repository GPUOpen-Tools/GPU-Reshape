#pragma once

// Layer
#include "SpvStream.h"
#include "SpvPhysicalBlockSource.h"

/// Single physical block
struct SpvPhysicalBlock {
    /// Original source
    SpvPhysicalBlockSource source;

    /// Final stream
    SpvStream stream;
};
