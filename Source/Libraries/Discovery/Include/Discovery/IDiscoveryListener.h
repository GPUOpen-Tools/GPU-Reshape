#pragma once

#include <Common/IComponent.h>

class IDiscoveryListener : public TComponent<IDiscoveryListener> {
public:
    COMPONENT(IDiscoveryListener);

    /// Starts this listener
    /// \return success state
    virtual bool Start() = 0;

    /// Stops this listener
    /// \return success state
    virtual bool Stop() = 0;

    /// Install this listener
    ///   ? Enables global hooking of respective discovery, always on for the end user
    /// \return success state
    virtual bool InstallGlobal() = 0;

    /// Uninstall this listener
    ///   ? Disables global hooking of respective discovery
    /// \return success state
    virtual bool UninstallGlobal() = 0;
};
