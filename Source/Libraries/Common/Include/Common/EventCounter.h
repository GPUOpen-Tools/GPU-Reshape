#pragma once

// Std
#include <condition_variable>
#include <mutex>

struct EventCounter {
    /// Wait until a given value is satisfied
    void Wait(uint64_t value) {
        std::unique_lock lock(mutex);
        var.wait(lock, [this, value] { return counter >= value; });
    }

    /// Increment the head (the value to be reached)
    void IncrementHead(uint64_t value = 1) {
        std::lock_guard lock(mutex);
        head += value;
    }

    /// Increment the counter (the completion value)
    void IncrementCounter(uint64_t value = 1) {
        std::lock_guard lock(mutex);
        counter += value;
        var.notify_one();
    }

    /// Reset this counter, not thread safe
    void Reset() {
        head = 0;
        counter = 0;
    }

    /// Get the current head value
    uint64_t GetHead() {
        std::lock_guard lock(mutex);
        return head;
    }

private:
    uint64_t                head{0};
    uint64_t                counter{0};
    std::mutex              mutex;
    std::condition_variable var;
};
