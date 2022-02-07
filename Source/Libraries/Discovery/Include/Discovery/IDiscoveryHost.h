#pragma once

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Forward declarations
class IDiscoveryListener;

class IDiscoveryHost : public TComponent<IDiscoveryHost> {
public:
    COMPONENT(IDiscoveryHost);

    /// Register a listener
    /// \param listener
    virtual void Register(const ComRef<IDiscoveryListener>& listener) = 0;

    /// Deregister a listener
    /// \param listener
    virtual void Deregister(const ComRef<IDiscoveryListener>& listener) = 0;

    /// Enumerate all listeners
    /// \param count if [listeners] is null, filled with the number of listeners
    /// \param listeners if not null, [count] elements are written to
    virtual void Enumerate(uint32_t* count, ComRef<IDiscoveryListener>* listeners) = 0;
};
