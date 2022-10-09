#include <Backends/DX12/Controllers/FeatureController.h>
#include <Backends/DX12/States/DeviceState.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>

// Backend
#include <Backend/IFeature.h>

// Schemas
#include <Schemas/Feature.h>

FeatureController::FeatureController(DeviceState* device) : device(device) {

}

bool FeatureController::Install() {
    // Install bridge
    bridge = registry->Get<IBridge>().GetUnsafe();
    if (!bridge) {
        return false;
    }

    // Install this listener
    bridge->Register(this);

    // Get info
    for (const ComRef<IFeature>& feature : device->features) {
        featureInfos.push_back(feature->GetInfo());
    }

    // OK
    return true;
}

void FeatureController::Uninstall() {
    // Uninstall this listener
    bridge->Deregister(this);
}

void FeatureController::Handle(const MessageStream *streams, uint32_t count) {
    std::lock_guard guard(mutex);

    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView view(streams[i]);

        // Visit all ordered messages
        for (ConstMessageStreamView<>::ConstIterator it = view.GetIterator(); it; ++it) {
            switch (it.GetID()) {
                case GetFeaturesMessage::kID: {
                    OnMessage(*it.Get<GetFeaturesMessage>());
                    break;
                }
            }
        }
    }
}

void FeatureController::OnMessage(const GetFeaturesMessage& message) {
    // Entry stream
    MessageStream descriptors;
    {
        MessageStreamView view(descriptors);

        // Fill descriptors
        for (size_t i = 0; i < featureInfos.size(); i++) {
            const FeatureInfo& info = featureInfos[i];

            // Allocate descriptor
            auto *descriptor = view.Add<FeatureDescriptorMessage>(FeatureDescriptorMessage::AllocationInfo{
                .nameLength = info.name.length(),
                .descriptionLength = info.description.length()
            });

            // Set data
            descriptor->name.Set(info.name);
            descriptor->description.Set(info.description);
            descriptor->featureBit = (1u << i);
        }
    }

    // Base stream
    MessageStreamView view(stream);

    // Allocate message
    auto *discovery = view.Add<FeatureListMessage>(FeatureListMessage::AllocationInfo{
        .descriptorsByteSize = descriptors.GetByteSize()
    });

    // Set entries
    discovery->descriptors.Set(descriptors);
}

void FeatureController::Commit() {
    std::lock_guard guard(mutex);

    // Export general to bridge
    bridge->GetOutput()->AddStreamAndSwap(stream);
}
