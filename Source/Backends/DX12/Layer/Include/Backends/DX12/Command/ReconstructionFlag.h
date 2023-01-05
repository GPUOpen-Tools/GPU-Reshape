#pragma once

// Common
#include "Common/Enum.h"

enum class ReconstructionFlag {
    /// Reconstruct the bound pipeline and binding information
    Pipeline = BIT(0),

    /// Reconstruct previous push constant data
    RootConstant = BIT(1),
};

BIT_SET(ReconstructionFlag);
