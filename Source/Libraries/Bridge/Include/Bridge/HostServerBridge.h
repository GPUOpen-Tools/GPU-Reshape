#pragma once

// Bridge
#include "MemoryBridge.h"
#include "EndpointConfig.h"

// Std
#include <thread>
#include <atomic>
#include <condition_variable>

// Forward declarations
class AsioHostServer;

/// Network Bridge
class HostServerBridge final : public IBridge {
public:
    ~HostServerBridge();

    /// Install this bridge
    /// \return success state
    bool Install(const EndpointConfig& config);

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
    /// Async read callback
    /// \param data the enqueued data
    /// \param size the enqueued size
    /// \return number of consumed bytes
    uint64_t OnReadAsync(const void* data, uint64_t size);

private:
    /// Current endpoint
    AsioHostServer* server{nullptr};

    /// Local storage
    OrderedMessageStorage storage;

    /// Piggybacked memory bridge
    MemoryBridge memoryBridge;

    /// Info across lifetime
    BridgeInfo info;

    /// Cache for commits
    std::vector<MessageStream> streamCache;
};
