#pragma once

// Std
#include <cstdint>

// Common
#include <Common/IComponent.h>

// Forward declarations
struct MessageStream;

class IBridgeListener : public IInterface {
public:
    virtual ~IBridgeListener() = default;

    /// Handle a batch of streams
    /// \param streams the streams, length of [count]
    /// \param count the number of streams
    virtual void Handle(const MessageStream* streams, uint32_t count) = 0;
};
