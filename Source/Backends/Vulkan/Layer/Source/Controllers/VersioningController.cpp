// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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

#include <Backends/Vulkan/Controllers/VersioningController.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>
#include <Backends/Vulkan/States/BufferState.h>
#include <Backends/Vulkan/States/ImageState.h>
#include <Backends/Vulkan/Translation.h>

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
    const char* formatStr = GetFormatString(state->createInfo.format);

    // Allocate version
    auto&& version = view.Add(ResourceVersionMessage::AllocationInfo {
        .nameLength = state->debugName ? std::strlen(state->debugName) : 0u,
        .formatLength =  std::strlen(formatStr)
    });

    // Fill info
    version->puid = state->virtualMappingTemplate.puid;
    version->version = head;
    version->name.Set(state->debugName ? state->debugName : "");
    version->width = state->createInfo.extent.width;
    version->height = state->createInfo.extent.height;
    version->depth = state->createInfo.extent.depth;
    version->format.Set(formatStr);
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
