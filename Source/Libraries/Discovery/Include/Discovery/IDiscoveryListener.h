#pragma once

#include <Common/IComponent.h>

class IDiscoveryListener : public IComponent {
public:
    COMPONENT(IDiscoveryListener);

    /// Install this listener
    /// \return success state
    virtual bool Install() = 0;
};
