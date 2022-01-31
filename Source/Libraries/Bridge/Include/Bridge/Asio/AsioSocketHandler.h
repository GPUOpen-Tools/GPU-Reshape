#pragma once

// Bridge
#include "Asio.h"

// Std
#include <functional>

/// Read delegate
using AsioReadDelegate = std::function<uint64_t(const void* data, uint64_t size)>;

/// Shared socket handler
class AsioSocketHandler {
public:
    /// The streaming buffer size
    static constexpr uint64_t kBufferSize = 1'000'000;

    /// Create from ASIO service
    /// \param ioService service
    AsioSocketHandler(asio::io_service &ioService) : socket(ioService) {
        /**/
    }

    /// Create from existing socket
    /// \param socket socket to be moved
    AsioSocketHandler(asio::ip::tcp::socket &&socket) : socket(std::move(socket)) {
        /**/
    }

    /// No copy or move
    AsioSocketHandler(const AsioSocketHandler&) = delete;
    AsioSocketHandler(AsioSocketHandler&&) = delete;
    AsioSocketHandler& operator=(const AsioSocketHandler&) = delete;
    AsioSocketHandler& operator=(AsioSocketHandler&&) = delete;

    /// Set the async read callback
    /// \param delegate
    void SetReadCallback(const AsioReadDelegate& delegate) {
        onRead = delegate;
    }

    /// Install this handler
    void Install() {
        Read();
    }

    /// Write async
    /// \param data data to be sent, lifetime bound to this call
    /// \param size byte count of data
    void WriteAsync(const void *data, uint64_t size) {
        ASSERT(socket.is_open(), "Socket lost");

        socket.async_write_some(
            asio::buffer(data, size),
            [this](const std::error_code &error, size_t bytes) {
                OnWrite(error, bytes);
            }
        );
    }

    /// Get the socket
    asio::ip::tcp::socket &Socket() {
        return socket;
    }

private:
    /// Start reading
    void Read() {
        ASSERT(socket.is_open(), "Socket lost");

        socket.async_read_some(
            asio::buffer(buffer, kBufferSize),
            [this](const std::error_code &error, size_t bytes) {
                OnRead(error, bytes);
            }
        );
    }

    /// Async read callback
    /// \param error error code
    /// \param bytes number of bytes read
    void OnRead(const std::error_code &error, size_t bytes) {
        ASSERT(!error, error.message().c_str());
        enqueuedBuffer.insert(enqueuedBuffer.end(), buffer, buffer + bytes);

        // Consume enqueued data
        if (onRead) {
            uint64_t consumed = onRead(enqueuedBuffer.data(), enqueuedBuffer.size());
            enqueuedBuffer.erase(enqueuedBuffer.begin(), enqueuedBuffer.begin() + consumed);
        }

        Read();
    }

    /// Async write callback
    /// \param error error code
    /// \param bytes number of bytes written
    void OnWrite(const std::error_code &error, size_t bytes) {
        ASSERT(!error, error.message().c_str());
    }

private:
    /// Underlying socket
    asio::ip::tcp::socket socket;

    /// Read callback
    AsioReadDelegate onRead;

    /// Current enqueued data
    std::vector<char> enqueuedBuffer;

    /// Streaming buffer
    char buffer[kBufferSize];
};
