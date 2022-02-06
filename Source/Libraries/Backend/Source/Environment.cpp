#include <Backend/Environment.h>
#include <Backend/EnvironmentInfo.h>
#include <Backend/FeatureHost.h>

// Bridge
#include <Bridge/MemoryBridge.h>
#include <Bridge/NetworkBridge.h>

// Common
#include <Common/Dispatcher/Dispatcher.h>
#include <Common/Plugin/PluginResolver.h>

using namespace Backend;

Environment::Environment() {
}

Environment::~Environment() {

}

bool Environment::Install(const EnvironmentInfo &info) {
    // Install the plugin resolver
    auto resolver = registry.AddNew<PluginResolver>();

    // Install the dispatcher
    registry.AddNew<Dispatcher>();

    // Install bridge
    if (info.memoryBridge) {
        // Intra process
        registry.AddNew<MemoryBridge>();
    } else {
        // Networked
        auto network = registry.AddNew<NetworkBridge>();

        // Attempt to install as server
        if (!network->InstallServer(EndpointConfig{})) {
            return false;
        }
    }

    // Install feature host
    registry.AddNew<FeatureHost>();

    // Need plugins?
    if (info.loadPlugins) {
        // Find all plugins
        if (!resolver->FindPlugins("backend", &plugins)) {
            return false;
        }

        // Install all found plugins
        resolver->InstallPlugins(plugins);
    }

    // OK
    return true;
}
