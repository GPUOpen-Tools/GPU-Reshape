#pragma once

// Std
#include <cstdint>

struct ExclusiveQueue {
    /// Parent family index
    uint32_t familyIndex{UINT32_MAX};

    /// Contained queue index
    uint32_t queueIndex{UINT32_MAX};
};
