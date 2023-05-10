#pragma once

// Layer
#include <Backends/DX12/Controllers/Versioning.h>
#include <Backends/DX12/Controllers/IController.h>

// Message
#include <Message/MessageStream.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/ComRef.h>

// Schemas
#include <Schemas/Versioning.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
class IBridge;
struct DeviceState;
struct ResourceState;

class VersioningController final : public IController, public IBridgeListener {
public:
    VersioningController(DeviceState* device);

    /// Install the controller
    bool Install();

    /// Uninstall the controller
    void Uninstall();

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) final;

    /// Commit all changes
    void Commit();

public:
    /// Create or commit a resource for versioning
    /// \param state resource state
    void CreateOrRecommitResource(ResourceState* state);

    /// Destroy a resource versioning
    /// \param state resource state
    void DestroyResource(ResourceState* state);

    /// Create a new branch at a potential segmentation point
    /// \return segmentation point
    VersionSegmentationPoint BranchOnSegmentationPoint();

    /// Collapse all branches for a given segmentation point
    /// \param versionSegPoint valid segmentation point
    void CollapseOnFork(VersionSegmentationPoint versionSegPoint);

protected:
    /// Commit a resource version
    /// \param view destination view
    /// \param state state to commit
    void CommitResourceVersion(MessageStreamView<ResourceVersionMessage>& view, ResourceState* state);

protected:
    /// Message handlers
    void OnMessage(const struct GetVersionSummarizationMessage& message);

private:
    DeviceState* device;

    /// Current head
    uint32_t head{0};

    /// Pending commit followed by branch?
    bool pendingCommit{false};
    
    /// Owning bridge, stored as naked pointer for referencing reasons
    IBridge* bridge{nullptr};

    /// Pending response stream
    MessageStream stream;

    /// Pending resource stream
    MessageStream resourceStream;

    /// Shared lock
    std::mutex mutex;
};
