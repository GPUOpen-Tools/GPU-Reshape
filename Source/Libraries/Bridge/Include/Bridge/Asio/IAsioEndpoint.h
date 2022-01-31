#pragma once

// Bridge
#include "AsioSocketHandler.h"

/// Endpoint abstraction
class IAsioEndpoint {
public:
    virtual ~IAsioEndpoint() = default;

    /// Set the read callback
    /// \param delegate
    virtual void SetReadCallback(const AsioReadDelegate& delegate) = 0;

    /// Write async
    /// \param data data to be sent, lifetime bound to this call
    /// \param size byte count of data
    virtual void WriteAsync(const void *data, uint64_t size) = 0;

    /// Start the endpoint service
    virtual void Run() = 0;
};
