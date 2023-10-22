#pragma once

// Std
#include <chrono>

class IntervalAction {
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
    
public:
    /// Constructor
    /// \param interval given interval, initial action excluded 
    IntervalAction(const std::chrono::milliseconds &interval) : interval(interval) {
        lastEvent = std::chrono::high_resolution_clock::now();
    }

    /// Step this action
    /// \return true if the inverval has passed
    bool Step() {
        TimePoint now = std::chrono::high_resolution_clock::now();

        // Pending interval?
        if (now - lastEvent < interval) {
            return false;
        }

        // Set last event
        lastEvent = now;
        return true;
    }

public:
    /// Create an action from a millisecond interval
    /// \param count number of milliseconds
    static IntervalAction FromMS(size_t count) {
        return IntervalAction(std::chrono::milliseconds(count));
    }

private:
    /// Last time this action was triggered
    TimePoint lastEvent;

    /// Constant interval
    std::chrono::milliseconds interval;
};
