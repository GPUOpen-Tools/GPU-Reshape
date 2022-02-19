#include <Backend/Environment.h>
#include <Backend/EnvironmentInfo.h>
#include <Backend/FeatureHost.h>

// Bridge
#include <Bridge/MemoryBridge.h>
#include <Bridge/HostServerBridge.h>
#include <Bridge/Network/PingPongListener.h>

// Services
#include <Services/HostResolver/HostResolverService.h>

// Schemas
#include <Schemas/PingPong.h>

// Common
#include <Common/Dispatcher/Dispatcher.h>
#include <Common/Plugin/PluginResolver.h>

using namespace Backend;

Environment::Environment() {
}

Environment::~Environment() {
    auto resolver = registry.Get<PluginResolver>();

    // Uninstall all plugins
    resolver->Uninstall();
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
        // Install the host resolver
        //  ? Ensures that the host resolver is running on the system
        HostResolverService hostResolverService;
        if (!hostResolverService.Install()) {
            return false;
        }

        // Networked
        auto network = registry.AddNew<HostServerBridge>();

        // Attempt to install as server
        if (!network->Install(EndpointConfig{})) {
            return false;
        }

        // Add default ping pong listener
        network->Register(PingPongMessage::kID, registry.New<PingPongListener>(network.GetUnsafe()));
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
