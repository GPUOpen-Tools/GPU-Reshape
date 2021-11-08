#pragma once

// Message
#include "Message/MessageStream.h"

// Std
#include <cstdint>

/// Contains the instrumentation info for a given object
struct InstrumentationInfo {
    /// The bit set of all active features
    uint64_t featureBitSet{0};

    /// Specialization stream
    MessageStream specialization;
};
