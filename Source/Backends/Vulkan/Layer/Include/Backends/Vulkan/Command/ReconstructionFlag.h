#pragma once

// Common
#include "Common/Enum.h"

enum class ReconstructionFlag {
    /// Reconstruct pipeline state and expected bindings
    Pipeline = BIT(0),

    /// Reconstruct push constant data
    PushConstant = BIT(1),

    /// Reconstruct render pass data
    RenderPass = BIT(2)
};

BIT_SET(ReconstructionFlag);
