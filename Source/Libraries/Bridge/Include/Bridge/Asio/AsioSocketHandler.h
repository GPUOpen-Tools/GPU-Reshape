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
#include "Asio.h"
#include "AsioDelegates.h"
#include "AsioConfig.h"

// Common
#include <Common/Assert.h>
#include <Common/GlobalUID.h>

// Std
#include <functional>

// Forward declarations
class AsioSocketHandler;

/// Read delegate
using AsioReadDelegate = std::function<uint64_t(AsioSocketHandler& handler, const void *data, uint64_t size)>;
using AsioErrorDelegate = std::function<bool(AsioSocketHandler& handler, const std::error_code& code, uint32_t repeatCount)>;

/// Shared socket handler
class AsioSocketHandler {
public:
    /// The streaming buffer size
    static constexpr uint64_t kBufferSize = 1'000'000;

    /// Create from ASIO service
    /// \param ioService service
    AsioSocketHandler(asio::io_service &ioService) : socket(ioService) {
        uuid = GlobalUID::New();

        // Create buffer
        buffer = std::make_unique<char[]>(kBufferSize);
    }

    /// No copy or move
    AsioSocketHandler(const AsioSocketHandler &) = delete;
    AsioSocketHandler(AsioSocketHandler &&) = delete;
    AsioSocketHandler &operator=(const AsioSocketHandler &) = delete;
    AsioSocketHandler &operator=(AsioSocketHandler &&) = delete;

    /// Set the async read callback
    /// \param delegate
    void SetReadCallback(const AsioReadDelegate &delegate) {
        onRead = delegate;
    }

    /// Set the async read callback
    /// \param delegate
    void SetErrorCallback(const AsioErrorDelegate &delegate) {
        onError = delegate;
    }

    /// Install this handler
    void Install() {
        Read();
    }

    /// Close this handler
    void Close() {
        socket.close();
    }

    /// Write synchronous
    /// \param data data to be sent, lifetime bound to this call
    /// \param size byte count of data
    bool WriteSync(const void *data, uint64_t size) {
        try {
            asio::write(socket, asio::buffer(data, size));
            return true;
        } catch (const asio::system_error&) {
            return false;
        }
    }

    /// Write async
    /// \param data data to be sent, lifetime bound to this call
    /// \param size byte count of data
    bool WriteAsync(const void *data, uint64_t size) {
        try {
#if ASIO_CONTENT_DEBUG
            fprintf(stdout, "AsioSocketHandler : Writing [");
            for (uint64_t i = 0; i < size; i++) {
                uint8_t byte = static_cast<const uint8_t*>(data)[i];
                fprintf(stdout, i == 0 ? "%i" : ", %i", static_cast<uint32_t>(byte));
            }
            fprintf(stdout, "]\n");
            fflush(stdout);
#endif 

            socket.async_write_some(
                asio::buffer(data, size),
                [this](const std::error_code &error, size_t bytes) {
                    OnWrite(error, bytes);
                }
            );

            return true;
        } catch (asio::system_error e) {
#if ASIO_DEBUG
            fprintf(stderr, "AsioSocketHandler : %s\n", e.what());
            fflush(stderr);
#endif

            return false;
        }
    }

    /// Set the GUID
    void SetGlobalUID(const GlobalUID& value) {
        uuid = value;
    }

    /// Check if the socket is open
    bool IsOpen() const {
        return socket.is_open();
    }

    /// Get the GUID
    const GlobalUID& GetGlobalUID() const {
        return uuid;
    }

    /// Get the socket
    asio::ip::tcp::socket &Socket() {
        return socket;
    }

private:
    /// Start reading
    void Read() {
        ASSERT(socket.is_open(), "Socket lost");

        try {
            socket.async_read_some(
                asio::buffer(buffer.get(), kBufferSize),
                [this](const std::error_code &error, size_t bytes) {
                    OnRead(error, bytes);
                }
            );
        } catch (asio::system_error) {
        }
    }

    /// Async read callback
    /// \param error error code
    /// \param bytes number of bytes read
    void OnRead(const std::error_code &error, size_t bytes) {
        if (!CheckError(error)) {
            return;
        }

        enqueuedBuffer.insert(enqueuedBuffer.end(), buffer.get(), buffer.get() + bytes);

        // Consume enqueued data
        if (onRead) {
            // Reduce removal until reads are done
            uint64_t consumptionHead = 0;

            // Consume all chunks possible
            for (; !enqueuedBuffer.empty();) {
                uint64_t consumed = onRead(*this, enqueuedBuffer.data() + consumptionHead, enqueuedBuffer.size() - consumptionHead);
                if (!consumed) {
                    break;
                }

                // Next!
                consumptionHead += consumed;
            }

            enqueuedBuffer.erase(enqueuedBuffer.begin(), enqueuedBuffer.begin() + consumptionHead);
        }

        Read();
    }

    /// Async write callback
    /// \param error error code
    /// \param bytes number of bytes written
    void OnWrite(const std::error_code &error, size_t bytes) {
        if (!CheckError(error)) {
            return;
        }
    }

    bool CheckError(const std::error_code& code) {
        if (!code) {
            errorRepeatCount = 0;
            return true;
        }

#if ASIO_DEBUG
        fprintf(stderr, "AsioSocketHandler : %s\n", code.message().c_str());
        fflush(stderr);
#endif

        if (onError) {
            return onError(*this, code, errorRepeatCount++);
        }

        return false;
    }

private:
    /// Underlying socket
    asio::ip::tcp::socket socket;

    /// UUID for this socket
    GlobalUID uuid;

    /// Delegates
    AsioReadDelegate onRead;
    AsioErrorDelegate onError;

    /// Numbers of successive errors
    uint32_t errorRepeatCount{0};

    /// Current enqueued data
    std::vector<char> enqueuedBuffer;

    /// Streaming buffer
    std::unique_ptr<char[]> buffer;
};
