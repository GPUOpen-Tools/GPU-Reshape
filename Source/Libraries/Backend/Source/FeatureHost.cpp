#include <Backend/FeatureHost.h>

void FeatureHost::Register(IFeature *feature) {
    features.push_back(feature);
}

void FeatureHost::Deregister(IFeature *feature) {
    auto&& it = std::find(features.begin(), features.end(), feature);
    if (it == features.end()) {
        features.erase(it);
    }
}

void FeatureHost::Enumerate(uint32_t *count, IFeature **_features) {
    if (_features) {
        std::copy(features.begin(), features.begin() + *count, _features);
    } else {
        *count = static_cast<uint32_t>(features.size());
    }
}
