#pragma once

// Backend
#include "IFeatureHost.h"

// Std
#include <vector>

class FeatureHost : public IFeatureHost {
public:
    void Register(IFeature *feature) override;
    void Deregister(IFeature *feature) override;
    void Enumerate(uint32_t *count, IFeature **features) override;

private:
    std::vector<IFeature*> features;
};
