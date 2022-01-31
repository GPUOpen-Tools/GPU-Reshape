#pragma once

// Bridge
#include "AsioSocketHandler.h"
#include "IAsioEndpoint.h"

// Std
#include <vector>

class AsioServer : public IAsioEndpoint {
public:
    /// Initialize this server
    /// \param port port to be used
    AsioServer(uint16_t port) : acceptor(ioService, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
        Accept();
    }

    /// Set the async read callback
    /// \param delegate
    void SetReadCallback(const AsioReadDelegate& delegate) override {
        onRead = delegate;
    }

    /// Set the async read callback
    /// \param delegate
    void SetErrorCallback(const AsioErrorDelegate& delegate) override {
        onError = delegate;
    }

    /// Check if the client is open
    bool IsOpen() override {
        return acceptor.is_open();
    }

    /// Connect to the endpoint
    /// \return success state
    bool Connect() override {
        if (acceptor.is_open()) {
            return true;
        }

        // TODO: Retry acceptor?
        return false;
    }

    /// Write async
    /// \param data data to be sent, lifetime bound to this call
    /// \param size byte count of data
    void WriteAsync(const void *data, uint64_t size) override {
        connections.erase(std::remove_if(connections.begin(), connections.end(), [data, size](const std::shared_ptr<Connection>& connection) {
            // Remove if closed
            if (!connection->handler.IsOpen()) {
                return true;
            }

            // Write to handler
            connection->handler.WriteAsync(data, size);
            return false;
        }), connections.end());
    }

    /// Run this server
    void Run() override {
        ioService.run();
    }

private:
    /// Connection state
    struct Connection {
        Connection(asio::io_service &io_service) : handler(io_service) {
            /* */
        }

        AsioSocketHandler handler;
    };

    /// Accept an ingoing connection
    void Accept() {
        // Create connection
        auto connection = std::make_shared<Connection>(ioService);

        // Start the accept
        acceptor.async_accept(connection->handler.Socket(), [this, connection](const std::error_code &error) {
            OnAccept(connection, error);
        });
    }

    /// Invoked during an accept
    /// \param connection the connection candidate
    /// \param error the error code, if any
    void OnAccept(const std::shared_ptr<Connection>& connection, const std::error_code &error) {
        // Success?
        if (!error) {
            // Add as valid connection
            connections.emplace_back(connection);

            // Set read
            if (onRead) {
                connection->handler.SetReadCallback(onRead);
            }

            // Begin listening
            connection->handler.Install();
        }

        // Accept next
        Accept();
    }

private:
    /// ASIO service
    asio::io_service ioService;

    /// All active connections
    std::vector<std::shared_ptr<Connection>> connections;

    /// Read delegate
    AsioReadDelegate onRead;
    AsioErrorDelegate onError;

    /// Acceptor helper
    asio::ip::tcp::acceptor acceptor;
};
