#pragma once

// Backend
#include "EventDataStack.h"
#include "CommandContextHandle.h"
#include "Command/CommandBuffer.h"

class CommandContext {
public:
    /// Given handle
    CommandContextHandle handle{kInvalidCommandContextHandle};

    /// Data to be pushed for the next event
    EventDataStack eventStack;

    /// Command buffer to be executed before invocation
    CommandBuffer buffer;
};
