#pragma once

// Common
#include "Delegate.h"

struct DispatcherJob {
    /// Userdata for this job
    void* userData{nullptr};

    /// Invoker
    Delegate<void(void* userData)> delegate;
};
