#include <Backend/FeatureHost.h>

void FeatureHost::Register(const ComRef<IComponentTemplate>& feature) {
    features.push_back(feature);
}

void FeatureHost::Deregister(const ComRef<IComponentTemplate>& feature) {
    auto&& it = std::find(features.begin(), features.end(), feature);
    if (it == features.end()) {
        features.erase(it);
    }
}

void FeatureHost::Enumerate(uint32_t *count, ComRef<IComponentTemplate>* _features) {
    if (_features) {
        std::copy(features.begin(), features.begin() + *count, _features);
    } else {
        *count = static_cast<uint32_t>(features.size());
    }
}
