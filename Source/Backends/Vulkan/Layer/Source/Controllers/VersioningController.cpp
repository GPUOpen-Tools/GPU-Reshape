#include <Backends/Vulkan/Controllers/VersioningController.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/States/ImageState.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>

// Schemas
#include <Schemas/Versioning.h>

VersioningController::VersioningController(DeviceDispatchTable *table) : table(table) {

}

bool VersioningController::Install() {
    // Install bridge
    bridge = registry->Get<IBridge>().GetUnsafe();
    if (!bridge) {
        return false;
    }

    // Install this listener
    bridge->Register(this);

    // OK
    return true;
}

void VersioningController::Uninstall() {
    // Uninstall this listener
    bridge->Deregister(this);
}

void VersioningController::Handle(const MessageStream *streams, uint32_t count) {
    std::lock_guard guard(mutex);

    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView view(streams[i]);

        // Visit all ordered messages
        for (ConstMessageStreamView<>::ConstIterator it = view.GetIterator(); it; ++it) {
            switch (it.GetID()) {
                case GetVersionSummarizationMessage::kID: {
                    OnMessage(*it.Get<GetVersionSummarizationMessage>());
                    break;
                }
            }
        }
    }
}

void VersioningController::OnMessage(const GetVersionSummarizationMessage& message) {
    // Ordered stream
    MessageStreamView view(stream);

    // Dynamic resource stream
    MessageStreamView<ResourceVersionMessage> resourceView(resourceStream);

    // Get linear views
    auto&& linearImages = table->states_image.GetLinear();
    auto&& linearBuffers = table->states_buffer.GetLinear();

    // Push response
    auto&& summarization = view.Add<VersionSummarizationMessage>();
    summarization->head = head;

    // Send image versions
    for (ImageState* state : linearImages) {
        CommitImageVersion(resourceView, state);
    }

    // Send buffer versions
    for (BufferState* state : linearBuffers) {
        CommitBufferVersion(resourceView, state);
    }
}

void VersioningController::CreateOrRecommitImage(ImageState *state) {
    std::lock_guard guard(mutex);
    pendingCommit = true;

    // Immediately commit
    MessageStreamView<ResourceVersionMessage> resourceView(resourceStream);
    CommitImageVersion(resourceView, state);
}

void VersioningController::CreateOrRecommitBuffer(BufferState *state) {
    std::lock_guard guard(mutex);
    pendingCommit = true;
    
    // Immediately commit
    MessageStreamView<ResourceVersionMessage> resourceView(resourceStream);
    CommitBufferVersion(resourceView, state);
}

void VersioningController::DestroyImage(ImageState *state) {
    std::lock_guard guard(mutex);
    pendingCommit = true;

    // Mark as deleted
    auto&& version = MessageStreamView<ResourceVersionMessage>(resourceStream).Add();
    version->puid = state->virtualMappingTemplate.puid;
    version->version = UINT32_MAX;
}

void VersioningController::DestroyBuffer(BufferState *state) {
    std::lock_guard guard(mutex);
    pendingCommit = true;

    // Mark as deleted
    auto&& version = MessageStreamView<ResourceVersionMessage>(resourceStream).Add();
    version->puid = state->virtualMapping.puid;
    version->version = UINT32_MAX;
}

VersionSegmentationPoint VersioningController::BranchOnSegmentationPoint() {
    std::lock_guard guard(mutex);

    // If no new resource states have been added / removed, no need to branch
    if (!pendingCommit) {
        return VersionSegmentationPoint {
            .id = head,
            .segmented = false
        };
    }
    
    // Export general to bridge
    // Ordering is guaranteed between streams, so we need to ensure the next ordered messages appear *after* this
    bridge->GetOutput()->AddStreamAndSwap(stream);
    bridge->GetOutput()->AddStreamAndSwap(resourceStream);

    // Enter new branch
    head++;
    
    // Ordered stream
    MessageStreamView view(stream);

    // Notify new branch
    auto&& branch = view.Add<VersionBranchMessage>();
    branch->head = head;

    // Cleanup
    pendingCommit = false;

    // Current segmentation point represents the last branch
    return VersionSegmentationPoint {
        .id = head - 1u,
        .segmented = true
    };
}

void VersioningController::CollapseOnFork(VersionSegmentationPoint versionSegPoint) {
    std::lock_guard guard(mutex);

    // Dont push collapse until it was segmented
    if (!versionSegPoint.segmented) {
        return;
    }
    
    // Ordered stream
    MessageStreamView view(stream);

    // Notify collapse event
    auto&& branch = view.Add<VersionCollapseMessage>();
    branch->head = versionSegPoint.id;
}

void VersioningController::CommitImageVersion(MessageStreamView<ResourceVersionMessage> &view, ImageState *state) {
    auto&& version = view.Add(ResourceVersionMessage::AllocationInfo {
        .nameLength = state->debugName ? std::strlen(state->debugName) : 0u
    });

    // Fill info
    version->puid = state->virtualMappingTemplate.puid;
    version->version = head;
    version->name.Set(state->debugName ? state->debugName : "");
    version->width = state->createInfo.extent.width;
    version->height = state->createInfo.extent.height;
    version->depth = state->createInfo.extent.depth;
}

void VersioningController::CommitBufferVersion(MessageStreamView<ResourceVersionMessage> &view, BufferState *state) {
    auto&& version = view.Add(ResourceVersionMessage::AllocationInfo {
        .nameLength = state->debugName ? std::strlen(state->debugName) : 0u
    });

    // Fill info
    version->puid = state->virtualMapping.puid;
    version->version = head;
    version->name.Set(state->debugName ? state->debugName : "");
    version->width = static_cast<uint32_t>(state->createInfo.size);
    version->height = 1u;
    version->depth = 1u;
}

void VersioningController::Commit() {
    std::lock_guard guard(mutex);

    // Export general to bridge
    bridge->GetOutput()->AddStreamAndSwap(stream);
    bridge->GetOutput()->AddStreamAndSwap(resourceStream);
}
