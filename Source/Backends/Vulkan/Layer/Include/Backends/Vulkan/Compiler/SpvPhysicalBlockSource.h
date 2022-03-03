#pragma once

// Std
#include <cstdint>

/// Block source
struct SpvPhysicalBlockSource {
    // TODO: Way, way overkill

    /// Program bounds
    const uint32_t* programBegin{nullptr};
    const uint32_t* programEnd{nullptr};

    /// Section bounds
    const uint32_t* code{nullptr};
    const uint32_t* end{nullptr};

    /// Source span
    IL::SourceSpan span;
};
