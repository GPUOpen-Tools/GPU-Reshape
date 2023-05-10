#pragma once

// Layer
#include <Backends/Vulkan/Controllers/IController.h>
#include <Backends/Vulkan/Controllers/Versioning.h>

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
struct DeviceDispatchTable;
struct ImageState;
struct BufferState;

class VersioningController final : public IController, public IBridgeListener {
public:
    VersioningController(DeviceDispatchTable* table);

    /// Install the controller
    bool Install();

    /// Uninstall the controller
    void Uninstall();

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) final;

    /// Commit all changes
    void Commit();

public:
    /// Create or recommit an image for versioning
    /// \param state image state
    void CreateOrRecommitImage(ImageState* state);

    /// Create or recommit a buffer for versioning
    /// \param state buffer state
    void CreateOrRecommitBuffer(BufferState* state);

    /// Destroy an image state versioning
    /// \param state image state
    void DestroyImage(ImageState* state);

    /// Destroy a buffer state versioning
    /// \param state buffer state
    void DestroyBuffer(BufferState* state);

    /// Create a branch point on potential segmentations
    /// \return segmentation point
    VersionSegmentationPoint BranchOnSegmentationPoint();

    /// Collapse all branches for a given segmentation point
    /// \param versionSegPoint segmentation point
    void CollapseOnFork(VersionSegmentationPoint versionSegPoint);

protected:
    /// Commit an image version
    /// \param view destination view
    /// \param state state to commit
    void CommitImageVersion(MessageStreamView<ResourceVersionMessage>& view, ImageState* state);

    /// Commit a buffer version
    /// \param view destination view
    /// \param state state to commit
    void CommitBufferVersion(MessageStreamView<ResourceVersionMessage>& view, BufferState* state);

protected:
    /// Message handlers
    void OnMessage(const struct GetVersionSummarizationMessage& message);

private:
    DeviceDispatchTable* table;

    /// Current head
    uint32_t head{0};

    /// Pending commit and branching?
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
