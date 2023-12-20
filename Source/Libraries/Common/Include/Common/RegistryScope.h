#pragma once

// Common
#include "Registry.h"

template<typename T = IComponent>
class RegistryScope {
public:
    /// Constructor
    /// \param registry registry to add the component
    /// \param component component to be added
    RegistryScope(Registry* registry, ComRef<T> component) : registry(registry), component(component) {
        registry->Add(component);
    }

    /// Destructor
    ~RegistryScope() {
        registry->Remove(component);
    }

    /// Accessor
    T* operator->() const {
        return component.GetUnsafe();
    }

private:
    /// Local registry
    Registry* registry;

    /// Added component
    ComRef<T> component;
};
