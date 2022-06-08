#pragma once

// Common
#include <Common/Dispatcher/Mutex.h>
#include <Common/Dispatcher/ConditionVariable.h>

struct Event {
    /// Wait for the event
    void Wait();

    /// Signal all listeners
    void Signal() {
        MutexGuard guard(mutex);
        state = true;
        var.NotifyAll();
    }

    /// Reset this event, not thread safe
    void Reset() {
        state = false;
    }

private:
    bool              state{false};
    Mutex             mutex;
    ConditionVariable var;
};
