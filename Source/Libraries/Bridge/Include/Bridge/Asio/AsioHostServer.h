// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include "AsioClient.h"
#include "AsioServer.h"
#include "AsioProtocol.h"
#include "AsioConfig.h"
#include "AsioAsyncRunner.h"

// Std
#include <memory>

/// Local server for remote client feedback, handled through the resolver
struct AsioHostServer {
    /// Constructor
    /// \param config the shared configuration
    /// \param info general information about the server
    AsioHostServer(const AsioConfig& config, const AsioHostClientInfo& info) : info(info), resolveClient(kAsioLocalhost, config.hostResolvePort) {
        resolveClient.SetReadCallback([this](AsioSocketHandler& handler, const void *data, uint64_t size) {
            return OnReadAsync(handler, data, size);
        });

        // Start runner
        resolveClientRunner.RunAsync(resolveClient);

        // Send allocation request
        AllocateToken(config.reservedToken);
    }

    /// Destructor
    ~AsioHostServer() {
        Stop();
    }

    /// Broadcast a message to all clients
    void BroadcastServerAsync(const void *data, uint64_t size) {
        if (!server) {
            return;
        }

        server->WriteAsync(data, size);
    }

    /// Is the resolver still open?
    bool IsOpen() {
        return resolveClient.IsOpen();
    }

    /// Stop the server
    void Stop() {
        resolveClient.Stop();
        resolveClientRunner.Stop();

        if (server) {
            server->Stop();
            serverRunner.Stop();
        }
    }

    /// Set the read callback
    /// \param delegate the event delegate
    void SetServerReadCallback(const AsioReadDelegate& delegate) {
        onRead = delegate;

        if (server) {
            server->SetReadCallback(delegate);
        }
    }

    /// Update the server info
    /// \param value host resolver info
    void UpdateInfo(const AsioHostClientInfo& value) {
        info = value;

        // Send request
        AsioHostClientResolverUpdate update;
        update.info = info;
        update.token = token;
        resolveClient.WriteAsync(&update, sizeof(update));
    }

protected:
    /// Allocate a new client token
    void AllocateToken(const AsioHostClientToken& reservedToken) {
        AsioHostClientResolverAllocate allocate;
        allocate.info = info;
        allocate.reservedToken = reservedToken;
        resolveClient.WriteAsync(&allocate, sizeof(allocate));
    }

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
            case AsioHostClientResolverAllocate::Response::kType:
                OnTokenResponse(handler, static_cast<const AsioHostClientResolverAllocate::Response*>(header));
                break;
            case AsioHostResolverClientRequest::ResolveServerRequest::kType:
                OnResolveServerRequest(handler, static_cast<const AsioHostResolverClientRequest::ResolveServerRequest *>(header));
                break;
        }

        // Consume
        return header->size;
    }

    /// Invoked on token responses (allocation events)
    /// \param handler responsible handler
    /// \param request the inbound request
    void OnTokenResponse(AsioSocketHandler& handler, const AsioHostClientResolverAllocate::Response* request) {
        token = request->token;
    }

    /// Invoked on server requests
    /// \param handler responsible handler
    /// \param request the inbound request
    void OnResolveServerRequest(AsioSocketHandler& handler, const AsioHostResolverClientRequest::ResolveServerRequest* request) {
        if (request->clientToken != token) {
            AsioHostResolverClientRequest::ServerResponse response;
            response.accepted = false;
            handler.WriteAsync(&response, sizeof(response));
            return;
        }

        // Create a server if first request
        if (!server) {
            CreateServer();
        }

        // Write response
        AsioHostResolverClientRequest::ServerResponse response;
        response.owner = request->owner;
        response.accepted = (server != nullptr);
        response.remotePort = server->GetPort();
        handler.WriteAsync(&response, sizeof(response));
    }

    /// Create the underlying server (on demand usage)
    void CreateServer() {
        // Create a new server with a system allocated port
        server = std::make_unique<AsioServer>(0);

        // Succeeded?
        if (!server->IsOpen()) {
            server.release();
        }

        // Set callbacks
        server->SetReadCallback(onRead);

        // Start the runner
        serverRunner.RunAsync(*server);
    }

private:
    AsioHostClientInfo info;

    /// Allocated token
    AsioHostClientToken token{};

    /// Resolve client
    AsioClient                  resolveClient;
    AsioAsyncRunner<AsioClient> resolveClientRunner;

    /// Delegates
    AsioReadDelegate onRead;
    AsioErrorDelegate onError;

    /// On demand server
    std::unique_ptr<AsioServer> server;
    AsioAsyncRunner<AsioServer> serverRunner;
};
