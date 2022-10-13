#pragma once

// Layer
#include <Backends/Vulkan/Controllers/IController.h>

// Message
#include <Message/MessageStream.h>

// Backend
#include <Backend/FeatureInfo.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
class IBridge;
struct DeviceDispatchTable;

class FeatureController final : public IController, public IBridgeListener {
public:
    COMPONENT(FeatureController);

    FeatureController(DeviceDispatchTable* table);

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
    DeviceDispatchTable* table;

    /// Owning bridge, stored as naked pointer for referencing reasons
    IBridge* bridge{nullptr};

    /// Pending response stream
    MessageStream stream;

    /// Pooled info
    std::vector<FeatureInfo> featureInfos;

    /// Shared lock
    std::mutex mutex;
};