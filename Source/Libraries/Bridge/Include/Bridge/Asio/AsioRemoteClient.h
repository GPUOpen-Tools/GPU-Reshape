#pragma once

// Bridge
#include "AsioClient.h"
#include "AsioProtocol.h"
#include "AsioConfig.h"
#include "AsioAsyncRunner.h"

// Common
#include <Common/EventHandler.h>

// Std
#include <memory>
#include <vector>
#include <mutex>
#include <string>

/// Delegates
using AsioRemoteServerDiscoveryDelegate = std::function<void(const AsioRemoteServerResolverDiscoveryRequest::Response& response)>;
using AsioRemoteServerResolveDelegate = std::function<void(const AsioHostResolverClientRequest::ResolveResponse& response)>;
using AsioRemoteServerConnectedDelegate = std::function<void(const AsioHostResolverClientRequest::ServerResponse& response)>;

/// Remote endpoint client for local server interfacing, managed by the resolver
struct AsioRemoteClient {
    /// Constructor
    AsioRemoteClient() {
        resolveClient.SetReadCallback([this](AsioSocketHandler& handler, const void *data, uint64_t size) {
            return OnReadResolveAsync(handler, data, size);
        });
    }

    /// Destructor
    ~AsioRemoteClient() {
        Stop();
    }

    /// Open a connection
    /// \param config endpoint config
    /// \return success state
    bool Connect(const AsioRemoteConfig& config) {
        // Cache endpoint info
        ipvxAddress = config.ipvxAddress;

        // Start synchronous connection
        bool result = resolveClient.Connect(config.ipvxAddress, config.hostResolvePort);

        // Start runner
        resolveClientRunner.RunAsync(resolveClient);

        // OK
        return result;
    }

    /// Asynchronously open a connection
    /// \param config endpoint config
    void ConnectAsync(const AsioRemoteConfig& config) {
        // Cache endpoint info
        ipvxAddress = config.ipvxAddress;

        // Start asynchronous connection
        resolveClient.ConnectAsync(config.ipvxAddress, config.hostResolvePort);

        // Start runner
        resolveClientRunner.RunAsync(resolveClient);
    }

    /// Set the connection delegate
    /// \param delegate
    void SetAsyncConnectedCallback(const AsioClientAsyncConnectedDelegate& delegate) {
        resolveClient.SetAsyncConnectedCallback(delegate);
    }

    /// Cancel ongoing requests
    void Cancel() {
        resolveClient.Cancel();
    }

    /// Stop this client
    void Stop() {
        resolveClient.Stop();

        // Stop the connected endpoint, optional
        if (endpointClient) {
            endpointClient->Stop();
        }

        // Stop the worker
        resolveClientRunner.Stop();
    }

    /// Send an async discovery request
    void DiscoverAsync() {
        AsioRemoteServerResolverDiscoveryRequest request;
        resolveClient.WriteAsync(&request, sizeof(request));
    }

    /// Send an async client request
    /// \param token client token
    void RequestClientAsync(const AsioHostClientToken& token) {
        AsioHostResolverClientRequest request;
        request.clientToken = token;
        resolveClient.WriteAsync(&request, sizeof(request));
    }

    /// Write to the connected client
    /// \param data data to be written
    /// \param size size of data
    void WriteAsync(const void* data, size_t size) {
        if (!endpointClient) {
            return;
        }

        // Send to endpoint
        endpointClient->WriteAsync(data, size);
    }

    /// Set the server wise read callback
    /// \param delegate the event delegate
    void SetServerReadCallback(const AsioReadDelegate& delegate) {
        onRead = delegate;

        if (endpointClient) {
            endpointClient->SetReadCallback(delegate);
        }
    }

    /// Set the server wise error callback
    /// \param delegate the event delegate
    void SetServerErrorCallback(const AsioErrorDelegate& delegate) {
        onError = delegate;

        if (endpointClient) {
            endpointClient->SetErrorCallback(delegate);
        }
    }

public:
    /// All events
    EventHandler<AsioRemoteServerDiscoveryDelegate> onDiscovery;
    EventHandler<AsioRemoteServerResolveDelegate> onResolve;
    EventHandler<AsioRemoteServerConnectedDelegate> onConnected;

protected:
    /// Invoked during async reads
    /// \param handler responsible handler
    /// \param data data pointer
    /// \param size size of data
    /// \return consumed bytes
    uint64_t OnReadResolveAsync(AsioSocketHandler& handler, const void *data, uint64_t size) {
        auto* header = static_cast<const AsioHeader*>(data);

        // Received full message?
        if (size < sizeof(AsioHeader) || size < header->size) {
            return 0;
        }

#if ASIO_DEBUG
        AsioDebugMessage(header);
#endif

        // Handle header
        switch (header->type) {
            default:
                break;
            case AsioRemoteServerResolverDiscoveryRequest::Response::kType:
                OnDiscovery(static_cast<const AsioRemoteServerResolverDiscoveryRequest::Response*>(header));
                break;
            case AsioHostResolverClientRequest::ResolveResponse::kType:
                OnResolverClientRequestResolveResponse(static_cast<const AsioHostResolverClientRequest::ResolveResponse*>(header));
                break;
            case AsioHostResolverClientRequest::ServerResponse::kType:
                OnResolverClientRequestServerResponse(static_cast<const AsioHostResolverClientRequest::ServerResponse*>(header));
                break;
        }

        // Consume
        return header->size;
    }

    /// Invoked on resolve responses
    /// \param handler responsible handler
    /// \param response the inbound response
    void OnResolverClientRequestResolveResponse(const AsioHostResolverClientRequest::ResolveResponse* response) {
        // Invoke subs
        onResolve.Invoke(*response);
    }

    /// Invoked on server responses
    /// \param handler responsible handler
    /// \param response the inbound response
    void OnResolverClientRequestServerResponse(const AsioHostResolverClientRequest::ServerResponse* response) {
        // Invoke subs on failure
        if (!response->accepted) {
            onConnected.Invoke(*response);
            return;
        }

        // Try to open the client
        ASSERT(!endpointClient, "Endpoint already opened");
        endpointClient = std::make_unique<AsioClient>(ipvxAddress.c_str(), response->remotePort);

        // Success?
        if (!endpointClient->IsOpen()) {
            endpointClient.release();
            return;
        }

        // Set callbacks
        endpointClient->SetReadCallback(onRead);
        endpointClient->SetErrorCallback(onError);

        // Start runner
        endpointClientRunner.RunAsync(*endpointClient);

        // Invoke subs
        onConnected.Invoke(*response);
    }

    /// Invoked on discovery responses
    /// \param handler responsible handler
    /// \param response the inbound response
    void OnDiscovery(const AsioRemoteServerResolverDiscoveryRequest::Response* response) {
        // Invoke subs
        onDiscovery.Invoke(*response);
    }

private:
    /// Shared lock
    std::mutex mutex;

    /// Endpoint ip address
    std::string ipvxAddress;

    /// The endpoint client
    std::unique_ptr<AsioClient> endpointClient;
    AsioAsyncRunner<AsioClient> endpointClientRunner;

    /// Delegates
    AsioReadDelegate onRead;
    AsioErrorDelegate onError;

    /// Resolve client
    AsioClient                  resolveClient;
    AsioAsyncRunner<AsioClient> resolveClientRunner;
};
