#include <Services/Discovery/DiscoveryService.h>

// Discovery
#include <Discovery/DiscoveryHost.h>
#include <Discovery/IDiscoveryListener.h>

DiscoveryService::~DiscoveryService() {
    resolver->Uninstall();

    // Cleanup registry
    registry.RemoveDestroy(resolver);
    registry.RemoveDestroy(host);
}

bool DiscoveryService::Install() {
    // Create the host
    host = registry.AddNew<DiscoveryHost>();

    // Create the resolver
    resolver = registry.AddNew<PluginResolver>();

    // Find all plugins
    PluginList list;
    if (!resolver->FindPlugins("discovery", &list)) {
        return false;
    }

    // Attempt to install all plugins
    if (!resolver->InstallPlugins(list)) {
        return false;
    }

    // Try to install the listeners
    if (!InstallListeners()) {
        return false;
    }

    // OK
    return true;
}

bool DiscoveryService::InstallListeners() {
    // Get the number of listeners
    uint32_t listenerCount{0};
    host->Enumerate(&listenerCount, nullptr);

    // Get all listeners
    std::vector<IDiscoveryListener*> listeners(listenerCount);
    host->Enumerate(&listenerCount, listeners.data());

    // Install all
    for (IDiscoveryListener* listener : listeners) {
        if (!listener->Install()) {
            return false;
        }
    }

    // OK
    return true;
}
