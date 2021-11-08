#pragma once

// Bridge
#include "IBridge.h"

// Message
#include <Message/OrderedMessageStorage.h>

/// In memory bridge
class MemoryBridge : public IBridge {
public:
    /// Overrides
    void Register(MessageID mid, IBridgeListener *listener) override;
    void Deregister(MessageID mid, IBridgeListener *listener) override;
    void Register(IBridgeListener *listener) override;
    void Deregister(IBridgeListener *listener) override;
    IMessageStorage *GetInput() override;
    IMessageStorage *GetOutput() override;
    void Commit() override;

private:
    /// Single stream for input & output
    OrderedMessageStorage sharedStorage;

    /// Caches
    std::vector<MessageStream> storageConsumeCache;
    std::vector<MessageStream> storageOrderedCache;

private:
    struct MessageBucket {
        /// All listeners for this message type
        std::vector<IBridgeListener*> listeners;
    };

    /// Message listener buckets
    std::map<MessageID, MessageBucket> buckets;

    /// Unspecialized listeners
    std::vector<IBridgeListener*> orderedListeners;
};
