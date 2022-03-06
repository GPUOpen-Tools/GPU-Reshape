#pragma once

// Common
#include <Common/Enum.h>

enum class BasicBlockFlag : uint32_t {
    None = 0x0,

    /// Do not perform instrumentation on this block
    NoInstrumentation = BIT(1),

    /// Block has been visited
    Visited = BIT(2),
};

BIT_SET(BasicBlockFlag);

enum class BasicBlockSplitFlag : uint32_t {
    None = 0x0,

    /// Perform patching on all branch users past the split point to the new block
    RedirectBranchUsers = BIT(1),

    /// All of the above
    All = ~0u
};

BIT_SET(BasicBlockSplitFlag);
