// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

// Backend
#include "AsioSocketHandler.h"

class AsioClient {
public:
    /// Initialize this client with a synchronous connection
    /// \param ipvxAddress ip address of the connecting endpoint
    /// \param port port number to be used
    AsioClient(const char* ipvxAddress, uint16_t port) : address(ipvxAddress), port(port), connection(ioService), resolver(ioService) {
        OpenConnection();
    }
    
    /// Initialize this client
    AsioClient() : connection(ioService), resolver(ioService) {
        
    }

    /// Destructor
    ~AsioClient() {
        ioService.stop();
    }

    /// Set the async read callback
    /// \param delegate
    void SetReadCallback(const AsioReadDelegate& delegate) {
        connection.SetReadCallback(delegate);
    }

    /// Set the async read callback
    /// \param delegate
    void SetErrorCallback(const AsioErrorDelegate& delegate) {
        connection.SetErrorCallback(delegate);
    }

    /// Set the async connection delegate
    /// \param delegate
    void SetAsyncConnectedCallback(const AsioClientAsyncConnectedDelegate& delegate) {
        asyncConnectedDelegate = delegate;
    }

    /// Write async
    /// \param data data to be sent, lifetime bound to this call
    /// \param size byte count of data
    void WriteAsync(const void *data, uint64_t size) {
        connection.WriteAsync(data, size);
    }

    /// Check if the client is open
    bool IsOpen() {
        return connection.IsOpen();
    }

    /// Connect to the endpoint
    /// \param ipvxAddress ip address
    /// \param port endpoint port
    /// \return success state
    bool Connect(const char* ipvxAddress, uint16_t port) {
        // Cache endpoint
        this->address = ipvxAddress;
        this->port = port;

        // Open synchronous
        return OpenConnection();
    }

    /// Asynchronously connect to the endpoint
    /// \param ipvxAddress ip address
    /// \param port endpoint port
    void ConnectAsync(const char* ipvxAddress, uint16_t port) {
        // Cache endpoint
        this->address = ipvxAddress;
        this->port = port;

        // Open asynchronous
        OpenConnectionAsync();
    }
    
    /// Try to open the connection
    /// \return success state
    bool OpenConnection() {
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

    /// Try to open an asynchronous connection
    void OpenConnectionAsync() {
        resolver.async_resolve(address.c_str(), std::to_string(port), [this](const std::error_code& code, const asio::ip::tcp::resolver::results_type& results) {
            OnAsyncResolve(code, results);
        });
    }

    /// Cancel an ongoing request
    void Cancel() {
        // Stop any resolver requests
        resolver.cancel();

        // Close existing socket
        if (connection.Socket().is_open()) {
            connection.Socket().cancel();
        }
    }

    /// Run this client
    void Run() {
        ioService.run();
    }

    /// Stop the client
    void Stop() {
        ioService.stop();
        connection.Close();
    }

private:
    /// Delegates
    AsioClientAsyncConnectedDelegate asyncConnectedDelegate;

private:
    /// Async resolve handler
    /// \param code
    /// \param results
    void OnAsyncResolve(const std::error_code& code, const asio::ip::tcp::resolver::results_type& results) {
        // Failed?
        if (code) {
            return;
        }

        // Connect to first valid endpoint
        asio::async_connect(connection.Socket(), results.begin(), results.end(), [this](const std::error_code& ec, auto it){
            // Start reading
            connection.Install();

            // Invoke handler
            if (asyncConnectedDelegate) {
                asyncConnectedDelegate();
            }
        });
    }

private:
    /// Cached address
    std::string address;

    /// Cached port
    uint16_t port{0};

private:
    /// ASIO service
    asio::io_service ioService;

    /// Shared handler
    AsioSocketHandler connection;

    /// Shared resolver
    asio::ip::tcp::resolver resolver;
};
