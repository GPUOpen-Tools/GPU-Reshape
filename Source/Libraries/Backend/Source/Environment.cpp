#include <Backend/Environment.h>
#include <Backend/EnvironmentInfo.h>
#include <Backend/FeatureHost.h>

// Bridge
#include <Bridge/MemoryBridge.h>

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
    auto* resolver = registry.AddNew<PluginResolver>();

    // Install the dispatcher
    registry.AddNew<Dispatcher>();

    // Install bridge
    if (info.bridge) {
        registry.Add(info.bridge);
    } else {
        // Default to memory bridge
        //  Will be changed to network bridge once that's up and working
        registry.AddNew<MemoryBridge>();
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
