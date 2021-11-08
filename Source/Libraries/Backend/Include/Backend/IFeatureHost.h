#pragma once

#include <Common/IComponent.h>

class IFeature;

class IFeatureHost : public IComponent {
public:
    COMPONENT(IFeatureHost);

    /// Register a feature
    /// \param feature
    virtual void Register(IFeature* feature) = 0;

    /// Deregister a feature
    /// \param feature
    virtual void Deregister(IFeature* feature) = 0;

    /// Enumerate all features
    /// \param count if [features] is null, filled with the number of features
    /// \param features if not null, [count] elements are written to
    virtual void Enumerate(uint32_t* count, IFeature** features) = 0;
};
