#pragma once

// Std
#include <condition_variable>
#include <mutex>

struct Event {
    /// Wait for the event
    void Wait() {
        std::unique_lock lock(mutex);
        var.wait(lock, [this] { return state; });
    }

    /// Signal all listeners
    void Signal() {
        std::lock_guard lock(mutex);
        state = true;
        var.notify_all();
    }

    /// Reset this event, not thread safe
    void Reset() {
        state = false;
    }

private:
    bool                    state{false};
    std::mutex              mutex;
    std::condition_variable var;
};
