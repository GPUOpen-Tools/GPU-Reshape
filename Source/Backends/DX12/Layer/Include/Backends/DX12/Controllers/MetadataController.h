#pragma once

// Layer
#include <Backends/DX12/Controllers/IController.h>

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
class ShaderCompiler;
struct DeviceState;
struct ReferenceObject;

class MetadataController final : public IController, public IBridgeListener {
public:
    COMPONENT(MetadataController);

    MetadataController(DeviceState* device);

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
    void OnMessage(const struct GetPipelineNameMessage& message);
    void OnMessage(const struct GetShaderCodeMessage& message);
    void OnMessage(const struct GetShaderILMessage& message);
    void OnMessage(const struct GetShaderBlockGraphMessage& message);
    void OnMessage(const struct GetObjectStatesMessage& message);
    void OnMessage(const struct GetShaderUIDRangeMessage& message);
    void OnMessage(const struct GetPipelineUIDRangeMessage& message);
    void OnMessage(const struct GetShaderSourceMappingMessage& message);

private:
    DeviceState* device;

    /// Owning bridge, stored as naked pointer for referencing reasons
    IBridge* bridge{nullptr};

    /// Components
    ComRef<ShaderCompiler> shaderCompiler;

    /// Pending response stream
    MessageStream stream;

    /// Pending segment mapping stream
    MessageStream segmentMappingStream;

    /// Shared lock
    std::mutex mutex;
};
