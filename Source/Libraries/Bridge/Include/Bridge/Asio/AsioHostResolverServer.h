// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#pragma once

// Bridge
#include "AsioServer.h"
#include "AsioProtocol.h"
#include "AsioConfig.h"
#include "AsioAsyncRunner.h"

// Common
#include <Common/EventHandler.h>

// Std
#include <memory>
#include <vector>
#include <mutex>

/// Delegates
using AsioClientAllocatedDelegate = std::function<void(const AsioHostClientInfo& info)>;

/// Handles handshake and proxying between host servers and remote clients
struct AsioHostResolverServer {
    /// Constructor
    /// \param config shared configuration
    AsioHostResolverServer(const AsioConfig& config) : server(config.hostResolvePort) {
        server.SetReadCallback([this](AsioSocketHandler& handler, const void *data, uint64_t size) {
            return OnReadAsync(handler, data, size);
        });

        // Client lost
        server.onClientLost.Add(0, [this](AsioSocketHandler& handler) {
            OnClientLost(handler);
        });

        runner.RunAsync(server);
    }

    /// Destructor
    ~AsioHostResolverServer() {
        Stop();
    }

    /// Stop the server
    void Stop() {
        server.Stop();
    }

    /// Check if the server is open
    bool IsOpen() {
        return server.IsOpen();
    }

    /// Get the underlying server
    AsioServer& GetServer() {
        return server;
    }

public:
    /// All events
    EventHandler<AsioClientAllocatedDelegate> onAllocated;

protected:
    /// Invoked during async reads
    /// \param handler responsible handler
    /// \param data data pointer
    /// \param size size of data
    /// \return consumed bytes
    uint64_t OnReadAsync(AsioSocketHandler& handler, const void *data, uint64_t size) {
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
            case AsioHostResolverClientRequest::kType:
                OnClientRequest(handler, static_cast<const AsioHostResolverClientRequest*>(header));
                break;
            case AsioHostClientResolverAllocate::kType:
                OnAllocateRequest(handler, static_cast<const AsioHostClientResolverAllocate*>(header));
                break;
            case AsioHostClientResolverDeallocate::kType:
                OnDeallocateRequest(handler, static_cast<const AsioHostClientResolverDeallocate*>(header));
                break;
            case AsioHostClientResolverUpdate::kType:
                OnAllocateUpdate(handler, static_cast<const AsioHostClientResolverUpdate*>(header));
                break;
            case AsioRemoteServerResolverDiscoveryRequest::kType:
                OnDiscoveryRequest(handler, static_cast<const AsioRemoteServerResolverDiscoveryRequest*>(header));
                break;
            case AsioHostResolverClientRequest::ServerResponse::kType:
                OnClientRequestServerResponse(handler, static_cast<const AsioHostResolverClientRequest::ServerResponse*>(header));
                break;
        }

        // Consume
        return header->size;
    }

    /// Invoked on client requests
    /// \param handler responsible handler
    /// \param request the inbound request
    void OnClientRequest(AsioSocketHandler& handler, const AsioHostResolverClientRequest* request) {
        std::lock_guard guard(mutex);

        // Response message
        AsioHostResolverClientRequest::ResolveResponse response;
        response.found = false;

        // Attempt to find handler
        if (std::shared_ptr<AsioSocketHandler> tokenHandler = server.GetSocketHandler(request->clientToken)) {
            // Write the resolver request to the server
            AsioHostResolverClientRequest::ResolveServerRequest serverRequest;
            serverRequest.clientToken = request->clientToken;
            serverRequest.owner = handler.GetGlobalUID();
            tokenHandler->WriteAsync(&serverRequest, sizeof(serverRequest));

            // Handled!
            response.found = true;
        }

        // Write response
        handler.WriteAsync(&response, sizeof(response));
    }

    /// Invoked on allocate requests
    /// \param handler responsible handler
    /// \param request the inbound request
    void OnAllocateRequest(AsioSocketHandler& handler, const AsioHostClientResolverAllocate* request) {
        std::lock_guard guard(mutex);

        // Did the client request a reserved token?
        if (request->reservedToken.IsValid()) {
            handler.SetGlobalUID(request->reservedToken);
        }

        // Create local client
        ClientInfo& info = clients.emplace_back();
        info.info = request->info;
        info.token = handler.GetGlobalUID();

        // Invoke listeners
        onAllocated.Invoke(info.info);

        // Write response
        AsioHostClientResolverAllocate::Response response;
        response.token = info.token;
        handler.WriteAsync(&response, sizeof(response));
    }

    /// Invoked on allocate requests
    /// \param handler responsible handler
    /// \param request the inbound request
    void OnDeallocateRequest(AsioSocketHandler& handler, const AsioHostClientResolverDeallocate* request) {
        std::lock_guard guard(mutex);

        // Find the handler
        auto it = std::find_if(clients.begin(), clients.end(), [request](const ClientInfo& candidate) {
            return candidate.token == request->token;
        });
        
        // Ignore if not found
        if (it == clients.end()) {
            return;
        }

        // Remove client, let the socket drop as needed
        clients.erase(it);
    }

    /// Invoked on allocate requests
    /// \param handler responsible handler
    /// \param request the inbound request
    void OnAllocateUpdate(AsioSocketHandler& handler, const AsioHostClientResolverUpdate* request) {
        std::lock_guard guard(mutex);

        // Find the handler
        auto it = std::find_if(clients.begin(), clients.end(), [request](const ClientInfo& candidate) {
            return candidate.token == request->token;
        });

        // Ignore if not found
        if (it == clients.end()) {
            return;
        }

        // Update info
        it->info = request->info;
    }

    /// Invoked on discovery requests
    /// \param handler responsible handler
    /// \param request the inbound request
    void OnDiscoveryRequest(AsioSocketHandler& handler, const AsioRemoteServerResolverDiscoveryRequest* request) {
        std::lock_guard guard(mutex);

        // Dynamic size of response
        uint64_t size = sizeof(AsioRemoteServerResolverDiscoveryRequest::Response) + sizeof(AsioRemoteServerResolverDiscoveryRequest::Entry) * clients.size();

        // Allocate message
        auto* message = new (malloc(size)) AsioRemoteServerResolverDiscoveryRequest::Response(size);
        message->entryCount = static_cast<uint32_t>(clients.size());

        // Fill clients
        for (size_t i = 0; i < clients.size(); i++) {
            message->entries[i].info = clients[i].info;
            message->entries[i].token = clients[i].token;
        }

#if ASIO_DEBUG
        fprintf(stdout, "Discovery response with %i entries, %ib\n", static_cast<uint32_t>(message->entryCount), static_cast<uint32_t>(size));
#endif

        // Write response
        handler.WriteAsync(message, size);

        // Cleanup
        free(message);
    }

    /// Invoked on request server responses
    /// \param handler responsible handler
    /// \param response the server response
    void OnClientRequestServerResponse(AsioSocketHandler& handler, const AsioHostResolverClientRequest::ServerResponse* response) {
        // Attempt to find handler
        std::shared_ptr<AsioSocketHandler> tokenHandler = server.GetSocketHandler(response->owner);
        if (!tokenHandler) {
            return;
        }

        // Proxy to handler
        tokenHandler->WriteAsync(response, sizeof(*response));
    }

    void OnClientLost(AsioSocketHandler& handler) {
        std::lock_guard guard(mutex);

        // Find the handler
        auto it = std::find_if(clients.begin(), clients.end(), [&handler](const ClientInfo& candidate) {
            return candidate.token == handler.GetGlobalUID();
        });

        // Consider the client lost
        if (it != clients.end()) {
            clients.erase(it);
        }
    }

private:
    struct ClientInfo {
        AsioHostClientToken token;
        AsioHostClientInfo info;
    };

    /// Shared lock
    std::mutex mutex;

    /// All connected clients
    std::vector<ClientInfo> clients;

    /// Underlying server
    AsioServer server;

    /// Async runner, tied to this instance
    AsioAsyncRunner<AsioServer> runner;
};
