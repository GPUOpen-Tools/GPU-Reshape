#include <Services/Discovery/DiscoveryService.h>

// Discovery
#include <Discovery/DiscoveryHost.h>
#include <Discovery/IDiscoveryListener.h>

DiscoveryService::~DiscoveryService() {
    resolver->Uninstall();
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

    // Get the number of listeners
    uint32_t listenerCount{0};
    host->Enumerate(&listenerCount, nullptr);

    // Get all listeners
    listeners.resize(listenerCount);
    host->Enumerate(&listenerCount, listeners.data());

    // OK
    return true;
}

bool DiscoveryService::Start() {
    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->Start()) {
            return false;
        }
    }

    // OK
    return true;
}

bool DiscoveryService::Stop() {
    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->Stop()) {
            return false;
        }
    }

    // OK
    return true;
}

bool DiscoveryService::InstallGlobal() {
    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->InstallGlobal()) {
            return false;
        }
    }

    // OK
    return true;
}

bool DiscoveryService::UninstallGlobal() {
    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->UninstallGlobal()) {
            return false;
        }
    }

    // OK
    return true;
}
