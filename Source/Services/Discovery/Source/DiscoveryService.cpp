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

bool DiscoveryService::IsGloballyInstalled() {
    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->IsGloballyInstalled()) {
            return false;
        }
    }

    // OK
    return true;
}

bool DiscoveryService::IsRunning() {
    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->IsRunning()) {
            return false;
        }
    }

    // OK
    return true;
}

bool DiscoveryService::Start() {
    bool anyFailed = false;

    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->Start()) {
            anyFailed = true;
        }
    }

    // OK
    return !anyFailed;
}

bool DiscoveryService::Stop() {
    bool anyFailed = false;

    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->Stop()) {
            anyFailed = true;
        }
    }

    // OK
    return !anyFailed;
}

bool DiscoveryService::InstallGlobal() {
    bool anyFailed = false;

    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->InstallGlobal()) {
            anyFailed = true;
        }
    }

    // OK
    return !anyFailed;
}

bool DiscoveryService::UninstallGlobal() {
    bool anyFailed = false;

    // Install all
    for (const ComRef<IDiscoveryListener>& listener : listeners) {
        if (!listener->UninstallGlobal()) {
            anyFailed = true;
        }
    }

    // OK
    return !anyFailed;
}
