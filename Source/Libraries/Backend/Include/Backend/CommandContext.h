#pragma once

// Backend
#include "EventDataStack.h"

class CommandContext {
public:
    /// Data to be pushed for the next event
    EventDataStack eventStack;
};
