#pragma once

// Backend
#include "AsioSocketHandler.h"
#include "IAsioEndpoint.h"

class AsioClient : public IAsioEndpoint {
public:
    /// Initialize this client
    /// \param ipvxAddress ip address of the connecting endpoint
    /// \param port port number to be used
    AsioClient(const char* ipvxAddress, uint16_t port) : connection(ioService), address(ipvxAddress), port(port) {
        OpenConnection();
    }

    /// Set the async read callback
    /// \param delegate
    void SetReadCallback(const AsioReadDelegate& delegate) override {
        connection.SetReadCallback(delegate);
    }

    /// Set the async read callback
    /// \param delegate
    void SetErrorCallback(const AsioErrorDelegate& delegate) override {
        connection.SetErrorCallback(delegate);
    }

    /// Write async
    /// \param data data to be sent, lifetime bound to this call
    /// \param size byte count of data
    void WriteAsync(const void *data, uint64_t size) override {
        connection.WriteAsync(data, size);
    }

    /// Check if the client is open
    bool IsOpen() override {
        return connection.IsOpen();
    }

    /// Connect to the endpoint
    /// \return success state
    bool Connect() override {
        if (connection.IsOpen()) {
            return true;
        }

        return OpenConnection();
    }

    /// Run this client
    void Run() override {
        ioService.run();
    }

private:
    /// Try to open the connection
    /// \return success state
    bool OpenConnection() {
        asio::ip::tcp::resolver resolver(ioService);

        // Attempt to resolve endpoint
        asio::ip::tcp::resolver::query    query(address.c_str(), std::to_string(port));
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
            return false;
        }

        // Install on connection
        connection.Install();
        return true;
    }

private:
    std::string address;

    uint16_t port;

private:
    /// ASIO service
    asio::io_service ioService;

    /// Shared handler
    AsioSocketHandler connection;
};
