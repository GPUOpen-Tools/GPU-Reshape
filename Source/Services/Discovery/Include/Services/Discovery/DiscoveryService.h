#pragma once

// Common
#include <Common/Plugin/PluginResolver.h>
#include <Common/Registry.h>

// Std
#include <vector>

// Forward declarations
class IDiscoveryHost;
class IDiscoveryListener;

class DiscoveryService {
public:
    ~DiscoveryService();

    /// Install this service
    /// \return success state
    bool Install();

    /// Check if all listeners are installed globally
    /// \return if all are installed, returns true
    bool IsGloballyInstalled();

    /// Check if all listeners are running
    /// \return if all are running, returns true
    bool IsRunning();

    /// Starts all listeners
    /// \return success state
    bool Start();

    /// Stops all listeners
    /// \return success state
    bool Stop();

    /// Install all listeners
    ///   ? Enables global hooking of respective discovery, always on for the end user
    /// \return success state
    bool InstallGlobal();

    /// Uninstall all listeners
    ///   ? Disables global hooking of respective discovery
    /// \return success state
    bool UninstallGlobal();

    /// Get the registry
    Registry* GetRegistry() {
        return &registry;
    }

private:
    /// Local registry
    Registry registry;

    /// Shared listener host
    ComRef<IDiscoveryHost> host{nullptr};

    /// Shared resolver
    ComRef<PluginResolver> resolver{nullptr};

    /// All listeners
    std::vector<ComRef<IDiscoveryListener>> listeners;
};
