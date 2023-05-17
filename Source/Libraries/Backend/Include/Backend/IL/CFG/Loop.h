#pragma once

// Common
#include <Common/Containers/TrivialStackVector.h>

namespace IL {
    // Forward declarations
    struct BasicBlock;

    struct Loop {
        /// Header of the natural loop
        BasicBlock* header{nullptr};

        /// All back edges
        TrivialStackVector<BasicBlock*, 4u> backEdges;
    };
}