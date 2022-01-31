#pragma once

// Message
#include "IMessageStorage.h"
#include "MessageStream.h"

// Std
#include <vector>
#include <map>

/// Simple batch ordered message storage
class OrderedMessageStorage final : public IMessageStorage {
public:
    /// Overrides
    void AddStream(const MessageStream &stream) override;
    void AddStreamAndSwap(MessageStream& stream) override;
    void ConsumeStreams(uint32_t *count, MessageStream *streams) override;
    void Free(const MessageStream& stream) override;
    uint32_t StreamCount() override;

private:
    /// Cache for message types
    struct MessageBucket {
        std::vector<MessageStream> freeStreams;
    };

    /// All message buckets
    std::map<MessageID, MessageBucket> messageBuckets;

    /// Free ordered streams, message invariant
    std::vector<MessageStream> freeOrderedStreams;

    /// Current pushed streams
    std::vector<MessageStream> storage;
};
