#pragma once

// Std
#include <Backend/IL/ID.h>

struct DXCodeOffsetTraceback {
    /// Originating basic block
    IL::ID basicBlockID{IL::InvalidID};

    /// Instruction index in basic block
    uint32_t instructionIndex{~0u};
};
