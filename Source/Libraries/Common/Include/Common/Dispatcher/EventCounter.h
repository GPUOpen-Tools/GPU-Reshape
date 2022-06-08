#pragma once

// Common
#include <Common/Dispatcher/Mutex.h>
#include <Common/Dispatcher/ConditionVariable.h>

struct EventCounter {
    /// Wait until a given value is satisfied
    void Wait(uint64_t value);

    /// Increment the head (the value to be reached)
    void IncrementHead(uint64_t value = 1) {
        MutexGuard guard(mutex);
        head += value;
    }

    /// Increment the counter (the completion value)
    void IncrementCounter(uint64_t value = 1) {
        MutexGuard guard(mutex);
        counter += value;
        var.NotifyOne();
    }

    /// Reset this counter, not thread safe
    void Reset() {
        head = 0;
        counter = 0;
    }

    /// Get the current head value
    uint64_t GetHead() {
        MutexGuard guard(mutex);
        uint64_t value = head;

        return value;
    }

private:
    uint64_t          head{0};
    uint64_t          counter{0};
    Mutex             mutex;
    ConditionVariable var;
};
