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
        onRead = delegate;
    }

    /// Set the async read callback
    /// \param delegate
    void SetErrorCallback(const AsioErrorDelegate& delegate) {
        onError = delegate;
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

            // Set read
            if (onRead) {
                connection->SetReadCallback(onRead);
            }

            // Set error
            if (onError) {
                connection->SetErrorCallback(onError);
            }

            // Begin listening
            connection->Install();

            // Invoke listeners
            onClientConnected.Invoke(*connection);
        }

        // Accept next
        Accept();
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
