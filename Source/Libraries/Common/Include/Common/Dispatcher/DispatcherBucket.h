#pragma once

// Common
#include <Common/Delegate.h>

// Std
#include <cstdint>
#include <atomic>

struct DispatcherBucket {
    /// Set the head counter
    void SetCounter(uint32_t count) {
        counter = count;
    }

    /// Increment the counter by a value
    void AddCounter(uint32_t count) {
        counter += count;
    }

    /// Increment the counter
    void Increment() {
        counter++;
    }

    /// Decrement the counter, signal if done
    void Decrement() {
        if (--counter == 0) {
            Signal();
        }
    }

    /// Get the current counter
    uint32_t GetCounter() const {
        return counter;
    }

    /// Signal the submitter
    void Signal() {
        completionFunctor.Invoke(userData);
    }

    /// User data for the functor
    void* userData{nullptr};

    /// User functor upon completion
    Delegate<void(void* userData)> completionFunctor;

private:
    std::atomic<uint32_t> counter{0};
};
