#pragma once

// Forward declarations
struct ConditionVariable;

// Std
#include <condition_variable>
#include <mutex>

/// CoreRT threading workaround
struct ConditionVariable {
    ConditionVariable();
    ~ConditionVariable();

    /// Notify one thread
    void NotifyOne();

    /// Notify all threads
    void NotifyAll();

    // Native helpers
#ifndef __cplusplus_cli
    std::condition_variable& Get() {
        return *var;
    }
#endif

#ifndef __cplusplus_cli
    std::condition_variable* var{nullptr};
#else
    void* opaque{nullptr};
#endif // __cplusplus_cli
};

