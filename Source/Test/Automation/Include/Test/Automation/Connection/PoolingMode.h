#pragma once

enum class PoolingMode {
    /// Store the message and release the task immediately
    StoreAndRelease,

    /// Replace the message, keep pooling for future matching messages
    Replace
};
