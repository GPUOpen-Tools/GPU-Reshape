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

private:
    std::vector<ComRef<IComponentTemplate>> features;
};
