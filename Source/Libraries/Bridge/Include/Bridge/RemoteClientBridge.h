#pragma once

// Bridge
#include "MemoryBridge.h"
#include "EndpointConfig.h"
#include "Asio/AsioProtocol.h"

// Std
#include <thread>
#include <atomic>

// Forward declarations
class AsioRemoteClient;

/// Network Bridge
class RemoteClientBridge final : public IBridge {
public:
    ~RemoteClientBridge();

    /// Install this bridge
    /// \return success state
    bool Install(const EndpointResolve& resolve);

    /// Send an async discovery request
    void DiscoverAsync();

    /// Send an async client request
    /// \param guid client guid, must originate from the discovery request
    void RequestClientAsync(const AsioHostClientToken& guid);

    /// Overrides
    void Register(MessageID mid, const ComRef<IBridgeListener>& listener) override;
    void Deregister(MessageID mid, const ComRef<IBridgeListener>& listener) override;
    void Register(const ComRef<IBridgeListener>& listener) override;
    void Deregister(const ComRef<IBridgeListener>& listener) override;
    IMessageStorage *GetInput() override;
    IMessageStorage *GetOutput() override;
    void Commit() override;

private:
    /// Invoked on client connections
    void OnConnected();

    /// Invoked on discovery responses
    /// \param response
    void OnDiscovery(const AsioRemoteServerResolverDiscoveryRequest::Response& response);

    /// Async read callback
    /// \param data the enqueued data
    /// \param size the enqueued size
    /// \return number of consumed bytes
    uint64_t OnReadAsync(const void* data, uint64_t size);

private:
    /// Current endpoint
    AsioRemoteClient* client{nullptr};

    /// Local storage
    OrderedMessageStorage storage;

    /// Piggybacked memory bridge
    MemoryBridge memoryBridge;

    /// Cache for commits
    std::vector<MessageStream> streamCache;
};
