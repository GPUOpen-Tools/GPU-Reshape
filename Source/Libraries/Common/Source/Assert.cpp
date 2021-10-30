#include <Common/Assert.h>

// Std
#include <stdio.h>

// Signal
#if defined(_MSC_VER)
#   include <Windows.h>
#else
#   include <csignal>
#endif

void Detail::Break(const char* message) {
    // Write with flush
    fprintf(stderr, "%s", message);
    fflush(stderr);

    // Invoke signal
#if defined(_MSC_VER)
    DebugBreak();
#else
    raise(SIGTRAP);
#endif
}
