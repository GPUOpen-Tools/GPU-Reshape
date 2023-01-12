#pragma once

// Bridge
#include "IBridge.h"

// Message
#include <Message/OrderedMessageStorage.h>

// Common
#include <Common/Dispatcher/Mutex.h>

/// In memory bridge
class MemoryBridge : public IBridge {
public:
    /// Overrides
    void Register(MessageID mid, const ComRef<IBridgeListener>& listener) override;
    void Deregister(MessageID mid, const ComRef<IBridgeListener>& listener) override;
    void Register(const ComRef<IBridgeListener>& listener) override;
    void Deregister(const ComRef<IBridgeListener>& listener) override;
    IMessageStorage *GetInput() override;
    IMessageStorage *GetOutput() override;
    BridgeInfo GetInfo() override;
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
        std::vector<ComRef<IBridgeListener>> listeners;
    };

    /// Shared lock
    Mutex mutex;

    /// Message listener buckets
    std::map<MessageID, MessageBucket> buckets;

    /// Unspecialized listeners
    std::vector<ComRef<IBridgeListener>> orderedListeners;
};
