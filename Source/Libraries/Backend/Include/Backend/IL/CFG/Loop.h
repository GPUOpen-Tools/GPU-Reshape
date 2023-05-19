#pragma once

// Common
#include <Common/Containers/TrivialStackVector.h>

namespace IL {
    // Forward declarations
    struct BasicBlock;

    struct Loop {
        /// Header of the natural loop
        BasicBlock* header{nullptr};

        /// All blocks within this loop
        TrivialStackVector<BasicBlock*, 16u> blocks;

        /// All exit blocks
        TrivialStackVector<BasicBlock*, 4u> exitBlocks;

        /// All back edges
        TrivialStackVector<BasicBlock*, 4u> backEdgeBlocks;
    };
}