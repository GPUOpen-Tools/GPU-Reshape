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

#include <Backends/Vulkan/Controllers/FeatureController.h>
#include <Backends/Vulkan/Tables/DeviceDispatchTable.h>

// Bridge
#include <Bridge/IBridge.h>

// Message
#include <Message/IMessageStorage.h>

// Backend
#include <Backend/IFeature.h>

// Common
#include <Common/Registry.h>

// Schemas
#include <Schemas/Feature.h>

FeatureController::FeatureController(DeviceDispatchTable* table) : table(table) {

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
    for (const ComRef<IFeature>& feature : table->features) {
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
            descriptor->featureBit = (1ull << i);
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
