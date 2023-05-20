#pragma once

enum class Queue {
    /// Standard graphics queue, supports all operations
    Graphics,

    /// Compute only queue
    Compute,

    /// Transfer queue, guaranteed to be exclusive from the instrumented device
    ExclusiveTransfer,

    /// Number of queues
    Count
};
