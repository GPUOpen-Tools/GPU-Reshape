#pragma once

// Backend
#include "Program.h"
#include "VisitFlag.h"

namespace IL {
    /// Visitation context
    struct VisitContext {
        /// Add a flag to this context
        /// \param value flag to be added
        void AddFlag(VisitFlagSet value) {
            flags |= value;
        }

        /// Current program for the object of interest
        IL::Program& program;

        /// Current function for the object of interest
        IL::Function& function;

        /// Current basic block for the object of interest
        IL::BasicBlock& basicBlock;

        /// Internal flags of this context
        VisitFlagSet flags{VisitFlag::Continue};
    };
}