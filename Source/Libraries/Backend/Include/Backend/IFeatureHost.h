#pragma once

// Common
#include <Common/ComRef.h>

// Forward declarations
class IComponentTemplate;

class IFeatureHost : public TComponent<IFeatureHost> {
public:
    COMPONENT(IFeatureHost);

    /// Register a feature
    /// \param feature
    virtual void Register(const ComRef<IComponentTemplate>& feature) = 0;

    /// Deregister a feature
    /// \param feature
    virtual void Deregister(const ComRef<IComponentTemplate>& feature) = 0;

    /// Enumerate all features
    /// \param count if [features] is null, filled with the number of features
    /// \param features if not null, [count] elements are written to
    virtual void Enumerate(uint32_t* count, ComRef<IComponentTemplate>* features) = 0;
};
