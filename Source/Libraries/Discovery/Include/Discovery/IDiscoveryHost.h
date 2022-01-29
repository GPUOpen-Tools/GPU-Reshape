#pragma once

// Common
#include <Common/IComponent.h>

// Forward declarations
class IDiscoveryListener;

class IDiscoveryHost : public IComponent {
public:
    COMPONENT(IDiscoveryHost);

    /// Register a listener
    /// \param listener
    virtual void Register(IDiscoveryListener* listener) = 0;

    /// Deregister a listener
    /// \param listener
    virtual void Deregister(IDiscoveryListener* listener) = 0;

    /// Enumerate all listeners
    /// \param count if [listeners] is null, filled with the number of listeners
    /// \param listeners if not null, [count] elements are written to
    virtual void Enumerate(uint32_t* count, IDiscoveryListener** listeners) = 0;
};
