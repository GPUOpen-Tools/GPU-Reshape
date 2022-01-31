#pragma once

// Message
#include <Message/MessageStream.h>

// Std
#include <mutex>
#include <string_view>

// Forward declarations
class IBridge;

class LogBuffer {
public:
    /// Add a new message
    ///   ? For formatting see LogFormat.h, separated for compile times
    /// \param system responsible system
    /// \param message message contents
    void Add(const std::string_view& system, const std::string_view& message);

    /// Commit all messages
    /// \param bridge
    void Commit(IBridge* bridge);

private:
    /// Shared mutex
    std::mutex mutex;

    /// Underlying stream
    MessageStream stream;
};
