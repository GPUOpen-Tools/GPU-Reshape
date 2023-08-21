#pragma once

// Std
#include <Backend/IL/ID.h>

struct SpvCodeOffsetTraceback {
    /// Originating basic block id
    IL::ID basicBlockID{IL::InvalidID};

    /// Linear instruction index in basic block
    uint32_t instructionIndex{~0u};
};
