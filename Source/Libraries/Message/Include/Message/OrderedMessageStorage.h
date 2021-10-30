#pragma once

// Message
#include "IMessageStorage.h"

// Std
#include <vector>

/// Simple batch ordered message storage
class OrderedMessageStorage final : public IMessageStorage {
public:
    /// Overrides
    void AddStream(const MessageStream &stream) override;
    void AddStreamAndSwap(MessageStream& stream) override;
    void ConsumeStreams(uint32_t *count, MessageStream *streams) override;
    void Free(const MessageStream& stream) override;

private:
    std::vector<MessageStream> freeStreams;
    std::vector<MessageStream> storage;
};
