#pragma once

// Forward declarations
struct Mutex;

// Std
#include <mutex>

/// CoreRT threading workaround
struct Mutex {
    Mutex();
    ~Mutex();

    /// Lock this mutex
    void Lock();

    /// Unlock this mutex
    void Unlock();

    // Native helpers
#ifndef __cplusplus_cli
    std::mutex& Get() {
        return *mutex;
    }
#endif

#ifndef __cplusplus_cli
    std::mutex* mutex{nullptr};
#else
    void* opaque{nullptr};
#endif // __cplusplus_cli
};

struct MutexGuard {
    MutexGuard(Mutex& mutex) : mutex(mutex) {
        mutex.Lock();
    }

    ~MutexGuard() {
        mutex.Unlock();
    }

private:
    Mutex& mutex;
};
