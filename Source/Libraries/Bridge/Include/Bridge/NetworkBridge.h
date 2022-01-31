#pragma once

// Bridge
#include "MemoryBridge.h"
#include "EndpointConfig.h"

// Std
#include <thread>
#include <atomic>
#include <condition_variable>

// Forward declarations
class IAsioEndpoint;

/// Network Bridge
class NetworkBridge final : public IBridge {
public:
    ~NetworkBridge();

    /// Install this bridge
    /// \return success state
    bool InstallServer(const EndpointConfig& config);

    /// Install this bridge
    /// \return success state
    bool InstallClient(const EndpointResolve& resolve);

    /// Overrides
    void Register(MessageID mid, IBridgeListener *listener) override;
    void Deregister(MessageID mid, IBridgeListener *listener) override;
    void Register(IBridgeListener *listener) override;
    void Deregister(IBridgeListener *listener) override;
    IMessageStorage *GetInput() override;
    IMessageStorage *GetOutput() override;
    void Commit() override;

private:
    /// Worker thread
    void Worker();

    /// Async read callback
    /// \param data the enqueued data
    /// \param size the enqueued size
    /// \return number of consumed bytes
    uint64_t OnReadAsync(const void* data, uint64_t size);

private:
    /// Current endpoint
    IAsioEndpoint* endpoint{nullptr};

    /// Local storage
    OrderedMessageStorage storage;

    /// Piggybacked memory bridge
    MemoryBridge memoryBridge;

    /// Cache for commits
    std::vector<MessageStream> streamCache;

    /// Worker
    std::thread workerThread;
};
