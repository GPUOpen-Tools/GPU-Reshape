#pragma once

// Common
#include "ComRef.h"

/// Component templating
class IComponentTemplate : public TComponent<IComponentTemplate> {
public:
    COMPONENT(IComponentTemplate);

    /// Instantiate the component
    /// \param registry registry to be instantiated against
    /// \return the instantiated component
    virtual ComRef<> Instantiate(Registry* registry) = 0;
};
