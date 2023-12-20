#pragma once

// Std
#include <condition_variable>
#include <mutex>

class MessageController {
public:
    /// Shared lock to this controller
    std::mutex mutex;

    /// Wake condition variable for acquisition
    std::condition_variable wakeCondition;

    /// Current commit id
    std::atomic<uint64_t> commitId{0};

    /// Is this controller pending release?
    /// Only safe from the controller owner
    bool pendingRelease{false};
};
