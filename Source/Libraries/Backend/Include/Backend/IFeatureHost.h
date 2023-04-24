#pragma once

// Common
#include <Common/ComRef.h>

// Forward declarations
class IComponentTemplate;
class IFeature;

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

    /// Install all features
    /// \param count if [features] is null, filled with the number of features
    /// \param features if not null, [count] elements are written to
    /// \param registry registry to be installed on
    /// \return success state
    virtual bool Install(uint32_t* count, ComRef<IFeature>* features, Registry* registry) = 0;
};
