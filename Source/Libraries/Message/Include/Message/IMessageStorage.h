#pragma once

// Message
#include "Message.h"

struct MessageStream;

/// Message stream storage
class IMessageStorage {
public:
    /// Add a stream
    /// \param stream
    virtual void AddStream(const MessageStream& stream) = 0;

    /// Add and swap a stream
    /// ? Inbound stream is consumed, and recycled with an older container
    /// \param stream
    virtual void AddStreamAndSwap(MessageStream& stream) = 0;

    /// Consume added streams
    /// \param count if [streams] is nullptr, filled with the number of consumable streams
    /// \param streams if not null, filled with [count] streams
    virtual void ConsumeStreams(uint32_t* count, MessageStream* streams) = 0;

    /// Free a consumed message stream
    /// \param stream
    virtual void Free(const MessageStream& stream) = 0;
};
