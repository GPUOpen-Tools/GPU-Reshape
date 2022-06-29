#pragma once

// Layer
#include <Backends/Vulkan/Controllers/IController.h>

// Message
#include <Message/MessageStream.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
class Registry;
class Dispatcher;
class IBridge;
struct DeviceDispatchTable;
struct ReferenceObject;

class MetadataController final : public IController, public IBridgeListener {
public:
    MetadataController(DeviceDispatchTable* table);

    /// Install the controller
    bool Install();

    /// Uninstall the controller
    void Uninstall();

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) final;

    /// Commit all changes
    void Commit();

protected:
    /// Message handlers
    void OnMessage(const struct GetShaderCodeMessage& message);
    void OnMessage(const struct GetShaderGUIDSMessage& message);

private:
    DeviceDispatchTable* table;

    /// Owning bridge, stored as naked pointer for referencing reasons
    IBridge* bridge{nullptr};

    /// Pending response stream
    MessageStream stream;

    /// Shared lock
    std::mutex mutex;
};
