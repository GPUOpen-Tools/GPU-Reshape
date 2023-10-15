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
#include "AsioSocketHandler.h"

// Common
#include <Common/EventHandler.h>

// Std
#include <vector>
#include <mutex>

/// Delegates
using AsioClientConnectedDelegate = std::function<void(AsioSocketHandler& handler)>;
using AsioClientLostDelegate = std::function<void(AsioSocketHandler& handler)>;

class AsioServer {
public:
    /// Initialize this server
    /// \param port port to be used
    AsioServer(uint16_t port) : acceptor(ioService, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
        Accept();
    }

    /// Destructor
    ~AsioServer() {
        ioService.stop();
    }

    /// Set the async read callback
    /// \param delegate
    void SetReadCallback(const AsioReadDelegate& delegate) {
        std::lock_guard guard(mutex);
        onRead = delegate;

        // Propagate to children
        for (const std::shared_ptr<AsioSocketHandler>& connection : connections) {
            connection->SetReadCallback(delegate);
        }
    }

    /// Check if the client is open
    bool IsOpen() {
        return acceptor.is_open();
    }

    /// Get the number of connections
    uint32_t GetConnectionCount() {
        std::lock_guard guard(mutex);
        return static_cast<uint32_t>(connections.size());
    }

    /// Write async
    /// \param data data to be sent, lifetime bound to this call
    /// \param size byte count of data
    void WriteAsync(const void *data, uint64_t size) {
        std::lock_guard guard(mutex);

        // Prune beforehand
        Prune();

        // Write to handlers
        for (const std::shared_ptr<AsioSocketHandler>& connection : connections) {
            connection->WriteAsync(data, size);
        }
    }

    /// Get a socket handler
    /// \param uuid the socket handler guid
    /// \return nullptr if not found
    std::shared_ptr<AsioSocketHandler> GetSocketHandler(const GlobalUID& uuid) {
        std::lock_guard guard(mutex);

        for (const std::shared_ptr<AsioSocketHandler>& connection : connections) {
            if (connection->GetGlobalUID() == uuid) {
                return connection;
            }
        }

        return nullptr;
    }

    /// Run this server
    void Run() {
        ioService.run();
    }

    /// Stop this server
    void Stop() {
        ioService.stop();
    }

    /// Get the allocated port
    uint16_t GetPort() const {
        return acceptor.local_endpoint().port();
    }

public:
    /// All events
    EventHandler<AsioClientConnectedDelegate> onClientConnected;
    EventHandler<AsioClientLostDelegate> onClientLost;

private:
    /// Prune all dead handlers
    void Prune() {
        connections.erase(std::remove_if(connections.begin(), connections.end(), [](const std::shared_ptr<AsioSocketHandler>& handler) {
            return !handler->IsOpen();
        }), connections.end());
    }

    /// Accept an ingoing connection
    void Accept() {
        // Create connection
        auto connection = std::make_shared<AsioSocketHandler>(ioService);

        // Start the accept
        acceptor.async_accept(connection->Socket(), [this, connection](const std::error_code &error) {
            OnAccept(connection, error);
        });
    }

    /// Invoked during an accept
    /// \param connection the connection candidate
    /// \param error the error code, if any
    void OnAccept(const std::shared_ptr<AsioSocketHandler>& connection, const std::error_code &error) {
        // Success?
        if (!error) {
            // Add as valid connection
            {
                std::lock_guard guard(mutex);
                connections.emplace_back(connection);
            }

            // Set shared error handler
            connection->SetErrorCallback([this](AsioSocketHandler& handler, const std::error_code& error, uint32_t repeatCount) {
                return OnError(handler, error, repeatCount);
            });

            // Set read
            if (onRead) {
                connection->SetReadCallback(onRead);
            }

            // Begin listening
            connection->Install();

            // Invoke listeners
            onClientConnected.Invoke(*connection);
        }

        // Accept next
        Accept();
    }

    bool OnError(AsioSocketHandler& handler, const std::error_code& error, uint32_t repeatCount) {
        // Incredibly naive handling
        if (repeatCount < 10) {
            return true;
        }

        std::lock_guard guard(mutex);

        // Find the handler
        auto it = std::find_if(connections.begin(), connections.end(), [&handler](const std::shared_ptr<AsioSocketHandler>& candidate) {
            return candidate->GetGlobalUID() == handler.GetGlobalUID();
        });

        // Consider the socket lost
        if (it != connections.end()) {
            onClientLost.Invoke(handler);
            connections.erase(it);
        }

        // Stop listening
        return false;
    }

private:
    /// ASIO service
    asio::io_service ioService;

    /// Shared lock
    std::mutex mutex;

    /// All active connections
    std::vector<std::shared_ptr<AsioSocketHandler>> connections;

    /// Read delegate
    AsioReadDelegate onRead;
    AsioErrorDelegate onError;

    /// Acceptor helper
    asio::ip::tcp::acceptor acceptor;
};
