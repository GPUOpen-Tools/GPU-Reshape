#pragma once

// Backend
#include "AsioSocketHandler.h"
#include "IAsioEndpoint.h"

class AsioClient : public IAsioEndpoint {
public:
    /// Initialize this client
    /// \param ipvxAddress ip address of the connecting endpoint
    /// \param port port number to be used
    AsioClient(const char* ipvxAddress, uint16_t port) : connection(ioService) {
        asio::ip::tcp::resolver resolver(ioService);

        // Attempt to resolve endpoint
        asio::ip::tcp::resolver::query    query(ipvxAddress, std::to_string(port));
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        asio::ip::tcp::resolver::iterator end;

        // Find first accepting endpoint
        while (endpoint_iterator != end) {
            connection.Socket().close();

            try {
                connection.Socket().connect(*endpoint_iterator++);
                break;
            } catch (...) {
                // Next
            }
        }

        // None available?
        if (!connection.Socket().is_open()) {
            return;
        }

        // Install on connection
        connection.Install();
    }

    /// Set the async read callback
    /// \param delegate
    void SetReadCallback(const AsioReadDelegate& delegate) override {
        connection.SetReadCallback(delegate);
    }

    /// Write async
    /// \param data data to be sent, lifetime bound to this call
    /// \param size byte count of data
    void WriteAsync(const void *data, uint64_t size) override {
        connection.WriteAsync(data, size);
    }

    /// Run this client
    void Run() override {
        ioService.run();
    }

private:
    /// ASIO service
    asio::io_service ioService;

    /// Shared handler
    AsioSocketHandler connection;
};
