#pragma once

// Common
#include <Common/Plugin/PluginResolver.h>
#include <Common/Registry.h>

// Forward declarations
class IDiscoveryHost;

class DiscoveryService {
public:
    ~DiscoveryService();

    /// Install this service
    /// \return success state
    bool Install();

    /// Get the registry
    Registry* GetRegistry() {
        return &registry;
    }

private:
    /// Install all listeners
    /// \return success state
    bool InstallListeners();

private:
    /// Local registry
    Registry registry;

    /// Shared listener host
    ComRef<IDiscoveryHost> host{nullptr};

    /// Shared resolver
    ComRef<PluginResolver> resolver{nullptr};
};
