#pragma once

// Backend
#include "IFeatureHost.h"

// Std
#include <vector>

class FeatureHost : public IFeatureHost {
public:
    void Register(const ComRef<IComponentTemplate>& feature) override;
    void Deregister(const ComRef<IComponentTemplate>& feature) override;
    void Enumerate(uint32_t *count, ComRef<IComponentTemplate> *features) override;
    bool Install(uint32_t *count, ComRef<IFeature> *features, Registry* registry) override;

private:
    std::vector<ComRef<IComponentTemplate>> features;
};
