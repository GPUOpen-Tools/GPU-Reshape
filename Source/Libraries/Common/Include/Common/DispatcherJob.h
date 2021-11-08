#pragma once

// Common
#include "Delegate.h"

// Forward declarations
struct DispatcherBucket;

struct DispatcherJob {
    /// Userdata for this job
    void* userData{nullptr};

    /// Invoker
    Delegate<void(void* userData)> delegate;

    /// Completion bucket
    DispatcherBucket* bucket{nullptr};
};
