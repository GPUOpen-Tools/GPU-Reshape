#pragma once

// Backend
#include "Program.h"

namespace Backend::IL {
    /// Get the true termination block
    /// \param program program to be fetched from
    /// \return termination block
    inline BasicBlock* GetTerminationBlock(Program& program) {
        IL::Function* entryPoint = program.GetEntryPoint();

        // Find return terminator
        for (IL::BasicBlock* block : entryPoint->GetBasicBlocks()) {
            if (block->GetTerminator()->Is<ReturnInstruction>()) {
                return block;
            }
        }

        // Ill-formed program
        ASSERT(false, "Ill-formed program, no block with return termination");
        return nullptr;
    }
}
