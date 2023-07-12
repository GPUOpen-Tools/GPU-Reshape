// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Backend/FeatureHost.h>
#include <Backend/IFeature.h>

// Common
#include <Common/IComponent.h>
#include <Common/IComponentTemplate.h>

// Std
#include <set>

void FeatureHost::Register(const ComRef<IComponentTemplate> &feature) {
    features.push_back(feature);
}

void FeatureHost::Deregister(const ComRef<IComponentTemplate> &feature) {
    auto &&it = std::find(features.begin(), features.end(), feature);
    if (it != features.end()) {
        features.erase(it);
    }
}

void FeatureHost::Enumerate(uint32_t *count, ComRef<IComponentTemplate> *_features) {
    if (_features) {
        std::copy(features.begin(), features.begin() + *count, _features);
    } else {
        *count = static_cast<uint32_t>(features.size());
    }
}

bool FeatureHost::Install(uint32_t *count, ComRef<IFeature> *_features, Registry *registry) {
    if (_features) {
        std::vector<std::pair<ComRef<IFeature>, FeatureInfo>> unsortedFeatures;

        // Install features
        for (const ComRef<IComponentTemplate> &_feature: features) {
            // Instantiate feature to this registry
            auto feature = Cast<IFeature>(_feature->Instantiate(registry));

            // Try to install feature
            if (!feature->Install()) {
                return false;
            }

            // Add with info
            unsortedFeatures.emplace_back(feature, feature->GetInfo());
        }

        // Current write offset
        uint32_t featureOffset = 0;

        // Set of installed components
        std::set<ComponentID> installedComponents;

        // Sorting loop
        for (; !unsortedFeatures.empty();) {
            bool mutated = false;

            // Try to accept all from end
            for (int64_t i = unsortedFeatures.size() - 1; i >= 0; i--) {
                const auto& [feature, info] = unsortedFeatures[i];

                // Validate that all dependencies are ready
                bool accepted = true;
                for (ComponentID dependency : info.dependencies) {
                    if (!installedComponents.contains(dependency)) {
                        accepted = false;
                        break;
                    }
                }

                // OK?
                if (!accepted) {
                    continue;
                }

                // Add id
                installedComponents.insert(feature->componentId);

                // Accept feature
                _features[featureOffset++] = feature;
                unsortedFeatures.erase(unsortedFeatures.begin() + i);

                // Mark as mutated
                mutated = true;
            }

            // Failed to insert?
            if (!mutated) {
                ASSERT(false, "Failed to sort features");
                return false;
            }
        }
    } else {
        *count = static_cast<uint32_t>(features.size());
    }

    // OK
    return true;
}
