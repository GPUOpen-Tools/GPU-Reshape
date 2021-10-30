#pragma once

namespace Detail {
    /// Invoke a signal
    void Break(const char* message);
}

/// Assertion
#if defined(NDEBUG)
#   define ASSERT(EXPRESSION, MESSAGE) (void)0
#else
#   define ASSERT(EXPRESSION, MESSAGE) if (!(EXPRESSION)) { Detail::Break(MESSAGE); } (void)0
#endif
