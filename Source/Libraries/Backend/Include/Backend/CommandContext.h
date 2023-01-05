#pragma once

// Backend
#include "EventDataStack.h"
#include "Command/CommandBuffer.h"

class CommandContext {
public:
    /// Data to be pushed for the next event
    EventDataStack eventStack;

    /// Command buffer to be executed before invocation
    CommandBuffer buffer;
};
