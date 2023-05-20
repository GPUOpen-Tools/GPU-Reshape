#pragma once

// Backend
#include "Queue.h"

// Common
#include <Common/IComponent.h>

// Forward declarations
struct CommandBuffer;

class IScheduler : public TComponent<IScheduler> {
public:
    COMPONENT(IScheduler);

    /// Schedule a command buffer
    /// \param queue queue to be scheduled on
    /// \param buffer buffer to be scheduled
    virtual void Schedule(Queue queue, const CommandBuffer& buffer) = 0;
};
