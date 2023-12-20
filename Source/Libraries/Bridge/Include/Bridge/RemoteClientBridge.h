// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
#include "MemoryBridge.h"
#include "EndpointConfig.h"
#include "Asio/AsioProtocol.h"
#include "Asio/AsioDelegates.h"

// Std
#include <thread>
#include <atomic>

// Forward declarations
struct AsioRemoteClient;

/// Network Bridge
class RemoteClientBridge final : public IBridge {
public:
    RemoteClientBridge();
    ~RemoteClientBridge();

    /// Install this bridge
    /// \return success state
    bool Install(const EndpointResolve& resolve);

    /// Install this bridge
    /// \return success state
    void InstallAsync(const EndpointResolve& resolve);

    /// Cancel pending requests
    void Cancel();

    /// Stop the connection
    void Stop();

    /// Send an async discovery request
    void DiscoverAsync();

    /// Send an async client request
    /// \param guid client guid, must originate from the discovery request
    void RequestClientAsync(const AsioHostClientToken& guid);

    /// Set the asynchronous connection delegate
    /// \param delegate delegate
    void SetAsyncConnectedDelegate(const AsioClientAsyncConnectedDelegate& delegate);

    /// Overrides
    void Register(MessageID mid, const ComRef<IBridgeListener>& listener) override;
    void Deregister(MessageID mid, const ComRef<IBridgeListener>& listener) override;
    void Register(const ComRef<IBridgeListener>& listener) override;
    void Deregister(const ComRef<IBridgeListener>& listener) override;
    IMessageStorage *GetInput() override;
    IMessageStorage *GetOutput() override;
    BridgeInfo GetInfo() override;
    void Commit() override;

    /// Enables auto commits on remote appends
    void SetCommitOnAppend(bool enabled)
    {
        commitOnAppend = enabled;
    }

private:
    /// Invoked on client connections
    /// \param response
    void OnConnected(const AsioHostResolverClientRequest::ServerResponse& response);

    /// Invoked on client resolves
    /// \param response
    void OnResolve(const AsioHostResolverClientRequest::ResolveResponse& response);

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

    /// Info across lifetime
    BridgeInfo info;

    /// Commit when streams are added
    bool commitOnAppend = false;

    /// Cache for commits
    std::vector<MessageStream> streamCache;
};
