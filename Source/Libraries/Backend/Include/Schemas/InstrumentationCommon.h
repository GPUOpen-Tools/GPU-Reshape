#pragma once

// Backend
#include <Schemas/Instrumentation.h>

/// Collapse operator
inline SetInstrumentationConfigMessage& operator|=(SetInstrumentationConfigMessage& lhs, const SetInstrumentationConfigMessage& rhs) {
    lhs.safeGuard |= rhs.safeGuard;
    lhs.detail |= rhs.detail;
    return lhs;
}
