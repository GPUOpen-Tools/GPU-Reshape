#include <Backends/DX12/Controllers/VersioningController.h>
#include <Backends/DX12/States/ResourceState.h>
#include <Backends/DX12/States/DeviceState.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>

// Schemas
#include <Schemas/Versioning.h>

VersioningController::VersioningController(DeviceState *device) : device(device) {

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

    // Get linear view
    auto&& linearResources = device->states_Resources.GetLinear();

    // Push response
    auto&& summarization = view.Add<VersionSummarizationMessage>();
    summarization->head = head;

    // Send resource versions
    for (ResourceState* state : linearResources) {
        CommitResourceVersion(resourceView, state);
    }
}

void VersioningController::CreateOrRecommitResource(ResourceState *state) {
    std::lock_guard guard(mutex);
    pendingCommit = true;

    // Immediately commit
    MessageStreamView<ResourceVersionMessage> resourceView(resourceStream);
    CommitResourceVersion(resourceView, state);
}

void VersioningController::DestroyResource(ResourceState *state) {
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

void VersioningController::CommitResourceVersion(MessageStreamView<ResourceVersionMessage> &view, ResourceState *state) {
    auto&& version = view.Add(ResourceVersionMessage::AllocationInfo {
        .nameLength = state->debugName ? std::strlen(state->debugName) : 0u
    });

    // Fill info
    version->puid = state->virtualMapping.puid;
    version->version = head;
    version->name.Set(state->debugName ? state->debugName : "");
    version->width = static_cast<uint32_t>(state->desc.Width);
    version->height = static_cast<uint32_t>(state->desc.Height);
    version->depth = static_cast<uint32_t>(state->desc.DepthOrArraySize);
}

void VersioningController::Commit() {
    std::lock_guard guard(mutex);

    // Export general to bridge
    bridge->GetOutput()->AddStreamAndSwap(stream);
    bridge->GetOutput()->AddStreamAndSwap(resourceStream);
}
