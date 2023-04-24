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
    if (it == features.end()) {
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
