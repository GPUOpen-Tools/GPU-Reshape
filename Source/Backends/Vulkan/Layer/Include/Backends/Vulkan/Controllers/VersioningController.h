// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
