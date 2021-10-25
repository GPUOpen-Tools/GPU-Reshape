#pragma once

// Backend
#include "Instruction.h"

namespace IL {
    struct Program {
        /// Allocate a new ID
        /// \return
        ID AllocID() {
            return counter++;
        }

    private:
        ID counter{0};
    };
}
