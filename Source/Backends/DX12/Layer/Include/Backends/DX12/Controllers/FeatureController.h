#pragma once

// Layer
#include <Backends/DX12/Controllers/IController.h>

// Message
#include <Message/MessageStream.h>

// Backend
#include <Backend/FeatureInfo.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/Allocator/Vector.h>
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
class IBridge;
struct DeviceState;

class FeatureController final : public IController, public IBridgeListener {
public:
    COMPONENT(FeatureController);

    FeatureController(DeviceState* device);

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
    void OnMessage(const struct GetFeaturesMessage& message);

private:
    DeviceState* device;

    /// Owning bridge, stored as naked pointer for referencing reasons
    IBridge* bridge{nullptr};

    /// Pending response stream
    MessageStream stream;

    /// Pooled info
    Vector<FeatureInfo> featureInfos;

    /// Shared lock
    std::mutex mutex;
};
